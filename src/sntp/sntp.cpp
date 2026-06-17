#include "sntp.hpp"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "time.h"

static const char *TAG = "sntp";

void sntp_start(void) {
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
  ESP_LOGI(TAG, "SNTP started !");
}

time_t sntp_wait_for_sync(void) {
  // Set time-zone to CET/CEST
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  ESP_LOGI(TAG, "Waiting for SNTP sync ... ...");
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  time_t now = time(NULL);
  ESP_LOGI(TAG, "Sync completed, unix time: %lld", (long long)now);
  // Format time to readable format
  char buf[32];
  struct tm *t = gmtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", t);
  ESP_LOGI(TAG, "Time: %s", buf);
  return now;
}
