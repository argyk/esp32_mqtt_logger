file(GLOB_RECURSE SRCS "**/*.c")
set(INCLUDES
    "."
    "main"
    "wifi"
    "mqtt"
    "i2c"
)

idf_component_register(SRCS ${SRCS}
                    REQUIRES esp_wifi nvs_flash esp_event esp-tls esp_driver_i2c
                    INCLUDE_DIRS ${INCLUDES}
                    EMBED_TXTFILES "mqtt/mosquitto.org.crt")
