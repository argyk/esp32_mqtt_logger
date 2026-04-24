#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"


// Configurations for MQTT client
// find local IP address using `ip addr` command, and update the MQTT_BROKER_URI accordingly
#define MQTT_BROKER_URI "mqtt://192.168.50.245:1883"
#define BROKER_CERTIFICATE_OVERRIDDEN 0
#define CERT_VALIDATE_MOSQUITTO_CA 0


void mqtt_init(void);

#endif // MQTT_H