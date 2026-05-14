#include "mqtt.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "mqtt_client.h"

#include <inttypes.h>

// Logging tag
static const char *TAG = "MQTT CLIENT";

static esp_mqtt_client_handle_t mqtt_client_handle = NULL;
static EventGroupHandle_t mqtt_event_group;

#if BROKER_CERTIFICATE_OVERRIDDEN
static const char cert_override_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    CONFIG_EXAMPLE_BROKER_CERTIFICATE_OVERRIDE "\n"
    "-----END CERTIFICATE-----";
#endif

#if CERT_VALIDATE_MOSQUITTO_CA
/* Embedded Mosquitto CA certificate for test.mosquitto.org:8883 */
// the asm symbols are created by the compiler due to EMBEDDED_FILES in CMakeLists.txt, and they point to the start and end of the binary data
extern const uint8_t mosquitto_org_crt_start[] asm("_binary_mosquitto_org_crt_start");
extern const uint8_t mosquitto_org_crt_end[] asm("_binary_mosquitto_org_crt_end");
#endif


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;  
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));

            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
                
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_publish(const char* topic, const char* data){
    if (mqtt_client_handle && (xEventGroupGetBits(mqtt_event_group) & MQTT_CONNECTED_BIT)){
        int ret = esp_mqtt_client_publish(mqtt_client_handle, topic, data, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", ret);
    }
}

EventGroupHandle_t mqtt_get_event_group(){
    return mqtt_event_group;
}

void mqtt_init(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI,
#if BROKER_CERTIFICATE_OVERRIDDEN
            .verification.certificate = cert_override_pem,
#elif CERT_VALIDATE_MOSQUITTO_CA
            .verification.certificate = (const char *)mosquitto_org_crt_start,

#endif
            // #else
            //.verification.crt_bundle_attach = esp_crt_bundle_attach, /* Use built-in certificate bundle */

        }
    };

    ESP_LOGI(TAG, "[MQTT_CLIENT_APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    mqtt_client_handle = esp_mqtt_client_init(&mqtt_cfg);

    mqtt_event_group = xEventGroupCreate();

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client_handle);

}