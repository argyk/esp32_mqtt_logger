#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "config.h"

static const char *TAG = "CONFIG";
static const char *NVS_NAMESPACE = "app_config";

// In-RAM cache populated by config_load()
static char s_wifi_ssid[64]     = DEFAULT_WIFI_SSID;
static char s_wifi_pass[64]     = DEFAULT_WIFI_PASS;
static char s_mqtt_broker1[128] = DEFAULT_MQTT_BROKER1;
static char s_mqtt_broker2[128] = DEFAULT_MQTT_BROKER2;
static uint32_t s_sensor_poll_ms = DEFAULT_SENSOR_POLL_MS;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static nvs_handle_t open_rw(esp_err_t *out_err)
{
    nvs_handle_t h;
    *out_err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    return h;
}

// Read a string from NVS into dst (max dst_len bytes including null terminator).
// Leaves dst unchanged when the key is absent or on any error.
static void read_str(nvs_handle_t h, const char *key, char *dst, size_t dst_len)
{
    size_t required = dst_len;
    esp_err_t err = nvs_get_str(h, key, dst, &required);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "key '%s' not in NVS — using default", key);
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_str('%s') failed: %s", key, esp_err_to_name(err));
    }
}

// Write a string to NVS and commit. Returns ESP_OK on success.
static esp_err_t write_str(const char *key, const char *value)
{
    esp_err_t err;
    nvs_handle_t h = open_rw(&err);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(h, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_str('%s') failed: %s", key, esp_err_to_name(err));
        nvs_close(h);
        return err;
    }

    err = nvs_commit(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
    }

    nvs_close(h);
    return err;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void config_load(void)
{
    esp_err_t err;
    nvs_handle_t h;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Namespace does not exist yet — first boot, all defaults apply
        ESP_LOGI(TAG, "NVS namespace not found — using compiled-in defaults");
        return;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    read_str(h, "wifi_ssid",   s_wifi_ssid,    sizeof(s_wifi_ssid));
    read_str(h, "wifi_pass",   s_wifi_pass,    sizeof(s_wifi_pass));
    read_str(h, "mqtt_broker1", s_mqtt_broker1, sizeof(s_mqtt_broker1));
    read_str(h, "mqtt_broker2", s_mqtt_broker2, sizeof(s_mqtt_broker2));

    // u32 example — different NVS call for numeric types
    uint32_t poll_ms = 0;
    err = nvs_get_u32(h, "poll_ms", &poll_ms);
    if (err == ESP_OK) {
        s_sensor_poll_ms = poll_ms;
    }

    nvs_close(h);
    ESP_LOGI(TAG, "config loaded — broker1=%s", s_mqtt_broker1);
}

esp_err_t config_set_wifi_ssid(const char *ssid)
{
    esp_err_t err = write_str("wifi_ssid", ssid);
    if (err == ESP_OK) {
        strlcpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid));
    }
    return err;
}

esp_err_t config_set_wifi_pass(const char *pass)
{
    esp_err_t err = write_str("wifi_pass", pass);
    if (err == ESP_OK) {
        strlcpy(s_wifi_pass, pass, sizeof(s_wifi_pass));
    }
    return err;
}

esp_err_t config_set_mqtt_broker1(const char *uri)
{
    esp_err_t err = write_str("mqtt_broker1", uri);
    if (err == ESP_OK) {
        strlcpy(s_mqtt_broker1, uri, sizeof(s_mqtt_broker1));
    }
    return err;
}

esp_err_t config_set_mqtt_broker2(const char *uri)
{
    esp_err_t err = write_str("mqtt_broker2", uri);
    if (err == ESP_OK) {
        strlcpy(s_mqtt_broker2, uri, sizeof(s_mqtt_broker2));
    }
    return err;
}

const char *config_get_wifi_ssid(void)    { return s_wifi_ssid; }
const char *config_get_wifi_pass(void)    { return s_wifi_pass; }
const char *config_get_mqtt_broker1(void) { return s_mqtt_broker1; }
const char *config_get_mqtt_broker2(void) { return s_mqtt_broker2; }
uint32_t    config_get_sensor_poll_ms(void) { return s_sensor_poll_ms; }
