# ESP32 MQTT Logger

@.claude/settings.local.json
@.claude/PURPOSE.md
@.claude/ROADMAP.md
@.claude/DONE.md

## Hardware

- Board: ESP32-S3 DevKitC-1
- I2C: SDA=GPIO8, SCL=GPIO9, 100 kHz
- MPU-6050 accelerometer/gyro at 0x69
- SSD1306 128×64 OLED at 0x3C
- DS3231 RTC at 0x68
- AT24C32 EEPROM (on DS3231 module) at 0x57
- BME680 environmental sensor at 0x76 (not yet integrated)
- SD card via SPI (not yet integrated)
- MQTT broker: `mqtt://192.168.50.245:1883` (local, plaintext)
  - desktop at 192.168.50.245, zenbook at 192.168.50.215

## Build & Flash

```sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
idf.py fullclean
```

## Conventions

- C++ for hardware drivers (RAII classes with init/deinit in constructor/destructor)
- C for WiFi and MQTT components (ESP-IDF C APIs, `extern "C"` boundaries)
- Each peripheral gets its own component under `src/<name>/` with a `.cpp`/`.hpp` pair
- Log tags match the component name: `static const char *TAG = "I2C"`
- Task stack default is 4096 bytes — only increase after confirming stack overflow via `uxTaskGetStackHighWaterMark`
- Use `ESP_ERROR_CHECK` for fatal init failures; don't add redundant error handling around it

## Instructions

- Always use the ESP-IDF new I2C master API (`i2c_master.h`), never the legacy driver
- Don't add comments that explain what the code does; only add them when the why is non-obvious
- When suggesting a new feature, tie it to a concept from PURPOSE.md
