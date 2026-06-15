#ifndef BME680_HPP
#define BME680_HPP

#include "bme68x.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct bme680_reading_t {
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
};

class BME680 {
public:
  BME680() : dev_handle(nullptr), bme_dev{}, conf{}, heatr{} {}
  ~BME680();
  bool init(i2c_master_bus_handle_t bus, QueueHandle_t oled_queue);
  bool read(bme680_reading_t &out);

  QueueHandle_t get_oled_queue_handle() const { return mOledQ; }

private:
  i2c_master_dev_handle_t dev_handle;
  bme68x_dev bme_dev;
  bme68x_conf conf;
  bme68x_heatr_conf heatr;

  QueueHandle_t mOledQ;
};

void bme680_task(void *param);

#endif // BME680_HPP
