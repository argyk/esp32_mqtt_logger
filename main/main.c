#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"


void wifi_init(void)
{
    // Initialize the network stack
    ESP_ERROR_CHECK(esp_netif_init());
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    
}

void app_main(void)
{
    wifi_init();
}
