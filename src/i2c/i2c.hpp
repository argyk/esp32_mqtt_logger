#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

// I2C configuration
#define I2C_MASTER_SCL_PIN GPIO_NUM_9     // S3: adjust if needed
#define I2C_MASTER_SDA_PIN GPIO_NUM_8     // S3: adjust if needed
#define I2C_MASTER_FREQ_HZ 100000        // 100 kHz
#define I2C_MASTER_TIMEOUT_MS 100

// Slave address (must match slave)
#define SLAVE_ESP32 0x55
#define SLAVE_MP6050 0x69
#define SLAVE_OLED 0x3C

// Command codes
#define CMD_GET_TEMPERATURE 0x01
#define CMD_GET_HUMIDITY 0x02

class I2CMaster {
    public:
        I2CMaster() : bus_handle(nullptr), dev_handle(nullptr), mp6050_handle(nullptr) {}
        ~I2CMaster();

        bool init(QueueHandle_t oled_queue);
        void scan();
        int read_humidity();
        float read_temperature();
        i2c_master_bus_handle_t get_bus_handle() const { return bus_handle; }
        i2c_master_dev_handle_t get_mp6050_handle() const { return mp6050_handle; }
        QueueHandle_t get_oled_queue_handle() const { return mOledQ; }

    private:
        i2c_master_bus_handle_t bus_handle;

        i2c_master_dev_handle_t dev_handle;
        i2c_master_dev_handle_t mp6050_handle;

        QueueHandle_t mOledQ;
};


void i2c_task(void *param);

#endif // I2C_HPP
