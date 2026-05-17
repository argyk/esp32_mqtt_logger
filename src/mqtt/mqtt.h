#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configurations for MQTT client
// find local IP address using `ip addr` command, and update the MQTT_BROKER_URI accordingly
// zenbook:215, desktop:245
#define MQTT_BROKER_URI "mqtt://192.168.50.245:1883"
#define BROKER_CERTIFICATE_OVERRIDDEN 0
#define CERT_VALIDATE_MOSQUITTO_CA 0

#define MQTT_CONNECTED_BIT      BIT0

void mqtt_init(void);
EventGroupHandle_t mqtt_get_event_group(void);
void mqtt_publish(const char* topic, const char* data);

#ifdef __cplusplus
}
#endif

#endif // MQTT_H