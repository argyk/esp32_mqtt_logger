#ifndef MPU6050_HPP
#define MPU6050_HPP

#include "driver/i2c_master.h"
#include "driver/i2c_types.h"

class MPU6050 {
public:
  MPU6050() : dev_handle(nullptr) {}
  ~MPU6050();

  bool init(i2c_master_bus_handle_t bus);
  i2c_master_dev_handle_t get_dev_handle() const { return this->dev_handle; };

private:
  i2c_master_dev_handle_t dev_handle;
};

void mpu6050_task(void *param);

#endif // MPU6050_HPP
