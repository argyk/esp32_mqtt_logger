#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "mqtt.h"
#include "i2c.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "MAIN";


void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(err);
}


extern "C" void app_main(void)
{
    nvs_init();
    if (wifi_init()) {
        mqtt_init();
    }

    I2CMaster i2c_master;
    if (!i2c_master.init()) {
        ESP_LOGE(TAG, "Failed to initialize I2C master");
        return;
    }


    
}
