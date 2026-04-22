#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"


// Configurations for MQTT client
#define MQTT_BROKER_URI "mqtts://test.mosquitto.org:1883"
#define BROKER_CERTIFICATE_OVERRIDDEN 0
#define CERT_VALIDATE_MOSQUITTO_CA 0


void mqtt_init(void);

#endif // MQTT_H