#include "oled.hpp"

#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"

static const char *TAG = "OLED";

// Draws text into the page-format framebuffer at the given page row (0-7) and
// column (0-127)
static void draw_string(uint8_t *buf, int col, int page, const char *str) {
  for (; *str && col < OLED_W; str++, col += 6) {
    uint8_t ch = (uint8_t)*str;
    if (ch < 0x20 || ch > 0x7E)
      ch = 0x20;
    const uint8_t *g = font5x8[ch - 0x20];
    for (int c = 0; c < 5 && col + c < OLED_W; c++) {
      buf[page * OLED_W + col + c] = g[c];
    }
    // col+5 stays 0 (spacing); buf is zeroed before use
  }
}

void oled_task(void *param) {
  oledTaskData *data = static_cast<oledTaskData *>(param);

  // Init OLED via esp_lcd (SSD1306 driver is built into IDF 6)
  esp_lcd_panel_io_handle_t io = nullptr;
  esp_lcd_panel_io_i2c_config_t io_cfg = {
      .dev_addr = SLAVE_OLED,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .control_phase_bytes = 1,
      .dc_bit_offset = 6,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .on_color_trans_done = nullptr,
      .user_ctx = nullptr,
      .flags = {},
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(data->bus_handle, &io_cfg, &io));
  ESP_LOGI(TAG, "OLED added to I2C bus");

  esp_lcd_panel_handle_t panel = nullptr;
  esp_lcd_panel_dev_config_t panel_cfg = {};
  panel_cfg.reset_gpio_num = GPIO_NUM_NC;
  panel_cfg.bits_per_pixel = 1;
  ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_cfg, &panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

  // Framebuffer in SSD1306 page format: buf[page * 128 + col]
  // Each byte = 8 vertical pixels (bit0 = top of the page row)
  static uint8_t buf[OLED_W * (OLED_H / 8)];
  memset(buf, 0, sizeof(buf));
  draw_string(buf, 0, 0, "Hello ESP32!");

  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, OLED_W, OLED_H, buf));
  oled_message msg;
  while (xQueueReceive(data->q, &msg, portMAX_DELAY)) {

    memset(buf, 0, sizeof(buf));
    draw_string(buf, 0, 0, "Hello ESP32!");

    if (msg.valid) {
      char line1[32], line2[32], line3[32], line4[32];
      snprintf(line1, sizeof(line1), "Temp: %.2fC", msg.temperature);
      snprintf(line2, sizeof(line2), "Hum:  %.2f%%", msg.humidity);
      snprintf(line3, sizeof(line3), "Pres: %.0fhPa", msg.pressure);
      snprintf(line4, sizeof(line4), "GasR: %.0fOhm", msg.gas_resistance);
      draw_string(buf, 0, 1, line1);
      draw_string(buf, 0, 2, line2);
      draw_string(buf, 0, 3, line3);
      draw_string(buf, 0, 4, line4);
    } else {
      draw_string(buf, 0, 1, "Sensor error");
    }

    esp_lcd_panel_draw_bitmap(panel, 0, 0, OLED_W, OLED_H, buf);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
