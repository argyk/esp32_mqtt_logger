#include "mpu6050.hpp"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "i2c.hpp"
#include "portmacro.h"

static const char *TAG = "MPU6050";

#define MPU6050_I2C_ADDR 0x69
#define I2C_TIMEOUT_MS 100

MPU6050::~MPU6050() {
  if (dev_handle)
    i2c_master_bus_rm_device(dev_handle);
}

bool MPU6050::init(i2c_master_bus_handle_t bus) {

  i2c_device_config_t mpu_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = MPU6050_I2C_ADDR,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 1000,
      .flags = {.disable_ack_check = false},
  };
  esp_err_t ret = i2c_master_bus_add_device(bus, &mpu_cfg, &dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add MPU-6050: %s", esp_err_to_name(ret));
    return false;
  }
  ESP_LOGI(TAG, "MPU-6050 added");

  uint8_t reg = 0x75;
  uint8_t who_am_i = 0;
  ret = i2c_master_transmit_receive(dev_handle, &reg, 1, &who_am_i, 1,
                                    I2C_MASTER_TIMEOUT_MS);

  if (ret != ESP_OK) {
    ESP_LOGE("MPU6050", "I2C error: %s", esp_err_to_name(ret));
  } else if (who_am_i != 0x68 && who_am_i != 0x70) {
    ESP_LOGW("MPU6050", "Unexpected WHO_AM_I = 0x%02X", who_am_i);
  } else {
    ESP_LOGI("MPU6050", "Device found! WHO_AM_I = 0x%02X", who_am_i);
  }

  return true;
}

void mpu6050_task(void *param) {
  // MPU6050 *sensor = static_cast<MPU6050 *>(param);
  vTaskDelay(portMAX_DELAY);
}
