#ifndef CONFIG_H
#define CONFIG_H

#include "esp_err.h"
#include "wifi_password.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compiled-in defaults — used when NVS keys are absent
#define DEFAULT_WIFI_SSID       ESP_WIFI_SSID
#define DEFAULT_WIFI_PASS       ESP_WIFI_PASS
#define DEFAULT_MQTT_BROKER1    "mqtt://192.168.50.245:1883"
#define DEFAULT_MQTT_BROKER2    "mqtt://192.168.50.215:1883"
#define DEFAULT_SENSOR_POLL_MS  2000

// Load all config from NVS, falling back to defaults where keys are missing.
// Call once in app_main after nvs_flash_init, before wifi_init / mqtt_init.
void config_load(void);

// Write a value to NVS. Returns ESP_OK on success.
// Useful for provisioning: call once, then the value persists across reboots.
esp_err_t config_set_wifi_ssid(const char *ssid);
esp_err_t config_set_wifi_pass(const char *pass);
esp_err_t config_set_mqtt_broker1(const char *uri);
esp_err_t config_set_mqtt_broker2(const char *uri);

// Read the in-RAM copy loaded by config_load. Never returns NULL.
const char *config_get_wifi_ssid(void);
const char *config_get_wifi_pass(void);
const char *config_get_mqtt_broker1(void);
const char *config_get_mqtt_broker2(void);
uint32_t    config_get_sensor_poll_ms(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
