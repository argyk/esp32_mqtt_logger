#include "i2c.hpp"
#include "cJSON.h"
#include "freertos/queue.h"
#include "mqtt.h"
#include "oled.hpp"
#include "time.h"

static const char *TAG = "I2C";

I2CMaster::~I2CMaster() {
  if (dev_handle)
    i2c_master_bus_rm_device(dev_handle);
  if (mp6050_handle)
    i2c_master_bus_rm_device(mp6050_handle);
  if (bus_handle)
    i2c_del_master_bus(bus_handle);
}

bool I2CMaster::init(QueueHandle_t oled_queue) {
  i2c_master_bus_config_t bus_cfg = {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = I2C_MASTER_SDA_PIN,
      .scl_io_num = I2C_MASTER_SCL_PIN,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags =
          {
              .enable_internal_pullup = true,
              .allow_pd = false,
          },
  };

  esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init I2C bus: %s", esp_err_to_name(ret));
    return false;
  }

  i2c_device_config_t dev_esp_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = SLAVE_ESP32,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 1000,
      .flags = {.disable_ack_check = false},
  };
  ret = i2c_master_bus_add_device(bus_handle, &dev_esp_cfg, &dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add slave ESP32: %s", esp_err_to_name(ret));
    return false;
  }
  ESP_LOGI(TAG, "ESP32 slave added");

  i2c_device_config_t mpu_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = SLAVE_MP6050,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 1000,
      .flags = {.disable_ack_check = false},
  };
  ret = i2c_master_bus_add_device(bus_handle, &mpu_cfg, &mp6050_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add MPU-6050: %s", esp_err_to_name(ret));
    return false;
  }
  ESP_LOGI(TAG, "MPU-6050 added");

  mOledQ = oled_queue;
  return true;
}

void I2CMaster::scan() {
  ESP_LOGI(TAG, "Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    esp_err_t ret =
        i2c_master_probe(bus_handle, addr, I2C_MASTER_TIMEOUT_MS / 2);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Device found at address 0x%02X", addr);
    }
  }
  ESP_LOGI(TAG, "I2C scan completed");
}

int I2CMaster::read_humidity() {
  uint8_t write_buf[1] = {CMD_GET_HUMIDITY};
  uint8_t read_buf[1] = {0};

  esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf),
                                      I2C_MASTER_TIMEOUT_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send humidity command: %s", esp_err_to_name(ret));
    return -1;
  }
  vTaskDelay(pdMS_TO_TICKS(20));
  ret = i2c_master_receive(dev_handle, read_buf, sizeof(read_buf),
                           I2C_MASTER_TIMEOUT_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to receive humidity: %s", esp_err_to_name(ret));
    return -1;
  }
  return read_buf[0];
}

float I2CMaster::read_temperature() {
  uint8_t write_buf[1] = {CMD_GET_TEMPERATURE};
  uint8_t read_buf[4];

  esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf),
                                      I2C_MASTER_TIMEOUT_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send temperature command: %s",
             esp_err_to_name(ret));
    return -999.0f;
  }
  vTaskDelay(pdMS_TO_TICKS(20));
  ret = i2c_master_receive(dev_handle, read_buf, sizeof(read_buf),
                           I2C_MASTER_TIMEOUT_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to receive temperature: %s", esp_err_to_name(ret));
    return -999.0f;
  }
  float temperature;
  std::memcpy(&temperature, read_buf, sizeof(float));
  return temperature;
}

void i2c_task(void *param) {
  I2CMaster *master = static_cast<I2CMaster *>(param);
  master->scan();

  // TODO: move to new folder for MPU-6050
  uint8_t reg = 0x75;
  uint8_t who_am_i = 0;
  esp_err_t ret =
      i2c_master_transmit_receive(master->get_mp6050_handle(), &reg, 1,
                                  &who_am_i, 1, I2C_MASTER_TIMEOUT_MS);

  if (ret != ESP_OK) {
    ESP_LOGE("MPU6050", "I2C error: %s", esp_err_to_name(ret));
  } else if (who_am_i != 0x68 && who_am_i != 0x70) {
    ESP_LOGW("MPU6050", "Unexpected WHO_AM_I = 0x%02X", who_am_i);
  } else {
    ESP_LOGI("MPU6050", "Device found! WHO_AM_I = 0x%02X", who_am_i);
  }
  EventGroupHandle_t mqtt_event_handle = mqtt_get_event_group();

  while (true) {
    float temperature = master->read_temperature();
    int humidity = master->read_humidity();

    if (temperature != -999.0f && humidity != -1) {
      oled_message oledMsg = {
          .temperature = temperature, .humidity = humidity, .valid = true};
      xQueueOverwrite(master->get_oled_queue_handle(), &oledMsg);

      if (xEventGroupGetBits(mqtt_event_handle) & MQTT_CONNECTED_BIT) {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
        cJSON_AddNumberToObject(json, "temperature", temperature);
        cJSON_AddNumberToObject(json, "humidity", humidity);
        char *payload = cJSON_PrintUnformatted(json);
        if (payload) {
          mqtt_publish("sensors/i2c", payload);
          cJSON_free(payload);
        }
        cJSON_Delete(json);
      }

      ESP_LOGI(TAG, "Temp: %.2f C, Humidity: %d%%", temperature, humidity);
    } else {
      oled_message oledMsg = {.temperature{}, .humidity{}, .valid = false};
      xQueueOverwrite(master->get_oled_queue_handle(), &oledMsg);
      ESP_LOGW(TAG, "Communication error");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
