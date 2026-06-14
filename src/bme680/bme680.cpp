#include "bme680.hpp"
#include "bme68x.h"
#include "bme68x_defs.h"
#include "cJSON.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/i2c_types.h"
#include "mqtt.h"
#include "time.h"
#include <cstdint>
#include <cstring>

static const char *TAG = "BME680";

#define BME680_I2C_ADDR 0x76
#define I2C_TIMEOUT_MS 100

extern "C" {

static BME68X_INTF_RET_TYPE bme_read(uint8_t reg_addr, uint8_t *reg_data,
                                     uint32_t length, void *intf_ptr) {
  auto dev = static_cast<i2c_master_dev_handle_t>(intf_ptr);
  esp_err_t ret = i2c_master_transmit_receive(dev, &reg_addr, 1, reg_data,
                                              length, I2C_TIMEOUT_MS);
  return ret == ESP_OK ? BME68X_OK : BME68X_E_COM_FAIL;
}

static BME68X_INTF_RET_TYPE bme_write(uint8_t reg_addr, const uint8_t *reg_data,
                                      uint32_t length, void *intf_ptr) {
  auto dev = static_cast<i2c_master_dev_handle_t>(intf_ptr);

  uint8_t buf[32];
  if (length + 1 > sizeof(buf))
    return BME68X_E_COM_FAIL;

  buf[0] = reg_addr;
  memcpy(buf + 1, reg_data, length);
  esp_err_t ret = i2c_master_transmit(dev, buf, length + 1, I2C_TIMEOUT_MS);
  return ret == ESP_OK ? BME68X_OK : BME68X_E_COM_FAIL;
}

static void bme_delay_us(uint32_t period, void *) { esp_rom_delay_us(period); }

} // extern "C"

BME680::~BME680() {
  if (dev_handle)
    i2c_master_bus_rm_device(dev_handle);
}

bool BME680::init(i2c_master_bus_handle_t bus) {
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = BME680_I2C_ADDR,
      .scl_speed_hz = 100000,
      .scl_wait_us = 1000,
      .flags = {.disable_ack_check = false},
  };
  if (i2c_master_bus_add_device(bus, &dev_cfg, &dev_handle) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add BME680 to I2C bus");
    return false;
  }

  bme_dev.intf = BME68X_I2C_INTF;
  bme_dev.intf_ptr = dev_handle;
  bme_dev.read = bme_read;
  bme_dev.write = bme_write;
  bme_dev.delay_us = bme_delay_us;
  bme_dev.amb_temp = 25;
  if (bme68x_init(&bme_dev) != BME68X_OK) {
    ESP_LOGE(TAG, "bme68x_init failed - check wiring and address 0x76");
    return false;
  }
  ESP_LOGI(TAG, "BME680 found, chip_id=0x%02X", bme_dev.chip_id);

  conf.filter = BME68X_FILTER_OFF;
  conf.odr = BME68X_ODR_NONE;
  conf.os_hum = BME68X_OS_2X;
  conf.os_temp = BME68X_OS_8X;
  conf.os_pres = BME68X_OS_4X;
  if (bme68x_set_conf(&conf, &bme_dev) != BME68X_OK) {
    ESP_LOGE(TAG, "bme68x_set_conf failed");
    return false;
  }

  heatr.enable = BME68X_ENABLE;
  heatr.heatr_temp = 320;
  heatr.heatr_dur = 159;
  if (bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr, &bme_dev) !=
      BME68X_OK) {
    ESP_LOGE(TAG, "bme68x_set_heatr_conf failed");
    return false;
  }

  return true;
}

bool BME680::read(bme680_reading_t &out) {
  if (bme68x_set_op_mode(BME68X_FORCED_MODE, &bme_dev) != BME68X_OK)
    return false;

  uint32_t meas_us = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme_dev) +
                     heatr.heatr_dur * 1000u;
  vTaskDelay(pdMS_TO_TICKS(meas_us / 1000 + 1));

  bme68x_data data = {};
  uint8_t n_fields = 0;
  if (bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme_dev) !=
          BME68X_OK ||
      n_fields == 0)
    return false;

  out.temperature = data.temperature;
  out.humidity = data.humidity;
  out.pressure = data.pressure;
  out.gas_resistance = data.gas_resistance;
  return true;
}

void bme680_task(void *param) {
  BME680 *sensor = static_cast<BME680 *>(param);
  EventGroupHandle_t mqtt_event = mqtt_get_event_group();

  while (true) {
    bme680_reading_t r = {};
    if (sensor->read(r)) {
      ESP_LOGI(TAG, "Temp: %.2fC, Hum: %.2f%%, Pres: %.2fhPa, Gas: %.0fΩ",
               r.temperature, r.humidity, r.pressure, r.gas_resistance);

      if (xEventGroupGetBits(mqtt_event) & MQTT_CONNECTED_BIT) {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
        cJSON_AddNumberToObject(json, "temperature", r.temperature);
        cJSON_AddNumberToObject(json, "humidity", r.humidity);
        cJSON_AddNumberToObject(json, "pressure", r.pressure);
        cJSON_AddNumberToObject(json, "gas_resistance", r.gas_resistance);
        char *payload = cJSON_PrintUnformatted(json);
        if (payload) {
          mqtt_publish("sensors/bme680", payload);
          cJSON_free(payload);
        }
        cJSON_Delete(json);
      }
    } else {
      ESP_LOGW(TAG, "read failed");
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
