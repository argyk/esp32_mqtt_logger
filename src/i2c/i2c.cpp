#include "i2c.hpp"

static const char *I2C_MASTER = "I2C_MASTER";

I2CMaster::~I2CMaster() {
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
    }
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
    }
}

bool I2CMaster::init() {
    // Initialize the I2C bus
    i2c_master_bus_config_t i2c_master_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_PIN,
        .scl_io_num = I2C_MASTER_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
            .allow_pd = false,
        },
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_master_cfg, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to initialize I2C bus: %s", esp_err_to_name(ret));
        return false;
    }


    // Add an I2C device to the bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SLAVE_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .scl_wait_us = 1000,
        .flags = {
            .disable_ack_check = false,
        },
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to add I2C device: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(I2C_MASTER, "I2C device added successfully");
    return true;
}


void I2CMaster::scan() {
    ESP_LOGI(I2C_MASTER, "Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t ret = i2c_master_probe(bus_handle, addr,I2C_MASTER_TIMEOUT_MS/2);
        if (ret == ESP_OK) {
            ESP_LOGI(I2C_MASTER, "Device found at address 0x%02X", addr);
        }
    }
    ESP_LOGI(I2C_MASTER, "I2C scan completed");
}

uint8_t I2CMaster::read_humidity() {
    uint8_t write_buf[1] = {CMD_GET_HUMIDITY};
    uint8_t read_buf[1] = {0};

    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to send humidity command: %s", esp_err_to_name(ret));
        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    ret = i2c_master_receive(dev_handle, read_buf, sizeof(read_buf), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to receive humidity: %s", esp_err_to_name(ret));
        return 0;
    }

    return read_buf[0];
}

float I2CMaster::read_temperature() {
    uint8_t write_buf[1] = {CMD_GET_TEMPERATURE};
    uint8_t read_buf[4];

    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to send temperature command: %s", esp_err_to_name(ret));
        return -999.0f;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    ret = i2c_master_receive(dev_handle, read_buf, sizeof(read_buf), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_MASTER, "Failed to receive temperature: %s", esp_err_to_name(ret));
        return -999.0f;
    }

    float temperature;
    std::memcpy(&temperature, read_buf, sizeof(float));
    return temperature;
}


void i2c_task(void *param) {
    I2CMaster *master = static_cast<I2CMaster *>(param);
    master->scan();

    while (true) {
        float temperature = master->read_temperature();
        uint8_t humidity = master->read_humidity();

        if (temperature != -999.0f && humidity != 0) {
            ESP_LOGI(I2C_MASTER, "Temperature: %.2f°C, Humidity: %d%%", temperature, humidity);
        } else {
            ESP_LOGW(I2C_MASTER, "Communication error");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Read every 1 second
    }
}