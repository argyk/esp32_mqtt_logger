file(GLOB_RECURSE SRCS "**/*.c")
set(INCLUDES
    "."
    "main"
    "wifi"
    "mqtt"
)

idf_component_register(SRCS ${SRCS}
                    REQUIRES esp_wifi nvs_flash esp_event esp-tls
                    INCLUDE_DIRS ${INCLUDES}
                    EMBED_TXTFILES "mqtt/mosquitto.org.crt")
