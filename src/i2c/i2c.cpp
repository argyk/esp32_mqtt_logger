#include "i2c.hpp"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt.h"
#include "oled.hpp"
#include "time.h"

static const char *TAG = "I2C";

I2CMaster::~I2CMaster() {
  if (bus_handle)
    i2c_del_master_bus(bus_handle);
}

bool I2CMaster::init() {
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

  this->scan();

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
