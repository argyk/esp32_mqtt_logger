#include "bme680.hpp"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "i2c.hpp"
#include "mpu6050.hpp"
#include "mqtt.h"
#include "nvs_flash.h"
#include "oled.hpp"
#include "sntp.hpp"
#include "wifi.h"

static const char *TAG = "MAIN";

void nvs_init(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
  ESP_ERROR_CHECK(err);
}

extern "C" void app_main(void) {
  nvs_init();
  config_load();
  if (wifi_init()) {
    sntp_start();
    mqtt_init();
    sntp_wait_for_sync();
  }

  static I2CMaster i2c_master;
  QueueHandle_t oledQ = xQueueCreate(1, sizeof(oled_message));

  if (!i2c_master.init()) {
    ESP_LOGE(TAG, "Failed to initialize I2C master");
    return;
  }

  static BME680 bme680;
  if (bme680.init(i2c_master.get_bus_handle(), oledQ)) {
    xTaskCreate(bme680_task, "bme680_task", 4096, &bme680, 5, nullptr);
  } else {
    ESP_LOGE(TAG, "BME680 init failed");
  }

  static MPU6050 mpu6050;
  if (mpu6050.init(i2c_master.get_bus_handle())) {
    xTaskCreate(mpu6050_task, "mpu6050_task", 4096, &mpu6050, 5, nullptr);
  } else {
    ESP_LOGE(TAG, "MPU6050 init failed");
  }

  static oledTaskData oledData = {.bus_handle = i2c_master.get_bus_handle(),
                                  .q = bme680.get_oled_queue_handle()};

  xTaskCreate(oled_task, "oled task", 4096, &oledData, 5, nullptr);
}
