#include "wifi.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *WIFI = "wifi station";
static EventGroupHandle_t wifi_event_group;
static int s_retry_num = 0;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAXIMUM_RETRY 10

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        esp_netif_dns_info_t dns = {0};
        dns.ip.u_addr.ip4.addr = 0x08080808u;
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        esp_netif_set_dns_info(esp_netif_get_default_netif(), ESP_NETIF_DNS_MAIN, &dns);
        ESP_LOGI(WIFI, "DNS set to 8.8.8.8");

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


bool wifi_init(void)
{
    ESP_LOGI(WIFI, "ESP_WIFI_MODE_STA");

    // freeRTOS event group to signal when we are connected, and to manage connection retries
    wifi_event_group = xEventGroupCreate();
    
    // Initialize the network stack
    ESP_ERROR_CHECK(esp_netif_init());
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#ifdef CONFIG_ESP_WIFI_WPA3_COMPATIBLE_SUPPORT
            .disable_wpa3_compatible_mode = 0,
#endif
        },
    };
    strlcpy((char *)wifi_config.sta.ssid,     config_get_wifi_ssid(), sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, config_get_wifi_pass(), sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI, "wifi_init finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI, "connected to SSID:%s", config_get_wifi_ssid());
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI, "Failed to connect to SSID:%s", config_get_wifi_ssid());
        return false;
    }

    ESP_LOGE(WIFI, "UNEXPECTED EVENT");
    return false;
}