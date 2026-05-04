#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <cstring>

// I2C configuration
#define I2C_MASTER_SCL_PIN GPIO_NUM_9     // S3: adjust if needed
#define I2C_MASTER_SDA_PIN GPIO_NUM_8     // S3: adjust if needed
#define I2C_MASTER_FREQ_HZ 400000        // 400 kHz
#define I2C_MASTER_TIMEOUT_MS 100

// Slave address (must match slave)
#define SLAVE_ADDRESS 0x55

// Command codes
#define CMD_GET_TEMPERATURE 0x01
#define CMD_GET_HUMIDITY 0x02

class I2CMaster {
    public:
        I2CMaster() : bus_handle(nullptr), dev_handle(nullptr) {}
        ~I2CMaster();

        bool init();
        void scan();
        uint8_t read_humidity();
        float read_temperature();

    private:
        i2c_master_bus_handle_t bus_handle;
        i2c_master_dev_handle_t dev_handle;
};

#endif // I2C_HPP
