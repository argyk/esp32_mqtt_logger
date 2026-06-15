#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"
#include "esp_log.h"
#include <cstring>

// I2C configuration
#define I2C_MASTER_SCL_PIN GPIO_NUM_9 // S3: adjust if needed
#define I2C_MASTER_SDA_PIN GPIO_NUM_8 // S3: adjust if needed
#define I2C_MASTER_FREQ_HZ 100000     // 100 kHz
#define I2C_MASTER_TIMEOUT_MS 100

// Slave address (must match slave)
#define SLAVE_MP6050 0x69
#define SLAVE_OLED 0x3C

class I2CMaster {
public:
  I2CMaster() : bus_handle(nullptr) {}
  ~I2CMaster();

  bool init();
  void scan();
  i2c_master_bus_handle_t get_bus_handle() const { return bus_handle; }

private:
  i2c_master_bus_handle_t bus_handle;
};

#endif // I2C_HPP
