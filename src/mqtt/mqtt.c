#include "mqtt.h"
#include "config.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include <string.h>

static const char *TAG = "MQTT CLIENT";

// Switch broker after this many consecutive disconnects without a successful
// connect
#define BROKER_FAIL_THRESHOLD 3

static esp_mqtt_client_handle_t mqtt_client_handle = NULL;
static EventGroupHandle_t mqtt_event_group;

static int s_active_broker = 0; // 0 = broker1, 1 = broker2
static int s_fail_count = 0;
static bool s_ever_connected = false;

static const char *active_uri(void) {
  return s_active_broker == 0 ? config_get_mqtt_broker1()
                              : config_get_mqtt_broker2();
}

static void switch_broker(void) {
  s_active_broker = (s_active_broker == 0) ? 1 : 0;
  s_fail_count = 0;
  s_ever_connected = false;
  ESP_LOGW(TAG, "switching to broker%d: %s", s_active_broker + 1, active_uri());

  esp_mqtt_client_config_t cfg = {
      .broker.address.uri = active_uri(),
  };
  esp_mqtt_set_config(mqtt_client_handle, &cfg);
  esp_mqtt_client_reconnect(mqtt_client_handle);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;

  switch ((esp_mqtt_event_id_t)event_id) {

  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "connected to %s", active_uri());
    s_ever_connected = true;
    s_fail_count = 0;
    xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
    break;

  case MQTT_EVENT_DISCONNECTED:
    xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);

    if (!s_ever_connected) {
      // Never connected on this broker — count as a failure
      s_fail_count++;
      ESP_LOGW(TAG, "broker%d failed to connect (%d/%d)", s_active_broker + 1,
               s_fail_count, BROKER_FAIL_THRESHOLD);

      if (s_fail_count >= BROKER_FAIL_THRESHOLD) {
        switch_broker();
      }
    } else {
      // Was connected, then dropped — let the client auto-retry this broker
      ESP_LOGI(TAG, "disconnected from %s — will retry", active_uri());
    }
    break;

  case MQTT_EVENT_ERROR:
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      ESP_LOGW(TAG, "transport error — errno %d (%s)",
               event->error_handle->esp_transport_sock_errno,
               strerror(event->error_handle->esp_transport_sock_errno));
    } else if (event->error_handle->error_type ==
               MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
      ESP_LOGW(TAG, "connection refused: 0x%x",
               event->error_handle->connect_return_code);
    }
    break;

  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "published msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_SUBSCRIBED:
  case MQTT_EVENT_UNSUBSCRIBED:
  case MQTT_EVENT_DATA:
    break;

  default:
    ESP_LOGD(TAG, "unhandled event id=%d", event->event_id);
    break;
  }
}

void mqtt_publish(const char *topic, const char *data) {
  if (mqtt_client_handle &&
      (xEventGroupGetBits(mqtt_event_group) & MQTT_CONNECTED_BIT)) {
    int msg_id =
        esp_mqtt_client_publish(mqtt_client_handle, topic, data, 0, 1, 0);
    ESP_LOGI(TAG, "publish queued msg_id=%d", msg_id);
  }
}

EventGroupHandle_t mqtt_get_event_group(void) { return mqtt_event_group; }

void mqtt_init(void) {
  mqtt_event_group = xEventGroupCreate();

  const esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = config_get_mqtt_broker1(),
      .session.last_will.topic = "sensors/status",
      .session.last_will.msg = "offline",
      .session.last_will.msg_len = strlen("offline"),
      .session.last_will.qos = 1,
      .session.last_will.retain = true,
  };

  ESP_LOGI(TAG, "connecting to %s", config_get_mqtt_broker1());
  mqtt_client_handle = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client_handle, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqtt_client_handle);
}
