file(GLOB_RECURSE SRCS "**/*.c" "**/*.cpp")
set(INCLUDES
    "."
    "main"
    "wifi"
    "mqtt"
    "i2c"
    "oled"
    "config"
    "sntp"
    "bme680"
    "mpu6050"
)

idf_component_register(SRCS ${SRCS}
                    REQUIRES 
                        esp_wifi 
                        nvs_flash 
                        esp_event
                        esp_driver_i2c 
                        driver 
                        esp_lcd
                        cjson
                    INCLUDE_DIRS ${INCLUDES})
