#include "mqtt.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"

#include <inttypes.h>

// Logging tag
static const char *MQTT = "mqtt client";


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
    ESP_LOGD(MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int ret;

    switch ((esp_mqtt_event_id_t)event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT, "MQTT_EVENT_CONNECTED");
            ret = esp_mqtt_client_subscribe(client, "/random/tmp0", 0);
            ESP_LOGI(MQTT, "sent subscribe successful, msg_id=%d", ret);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

            ret = esp_mqtt_client_publish(client, "/random/tmp0", "data", 0, 1, 0);
            ESP_LOGI(MQTT, "sent publish successful, msg_id=%d", ret);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;  
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT, "MQTT_EVENT_ERROR");

            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(MQTT, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(MQTT, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(MQTT, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));

            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(MQTT, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
                
            } else {
                ESP_LOGW(MQTT, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;

        default:
            ESP_LOGI(MQTT, "Other event id:%d", event->event_id);
            break;
    }
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

        },
    };

    ESP_LOGI(MQTT, "[MQTT_CLIENT_APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);



    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}