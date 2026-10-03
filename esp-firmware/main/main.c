#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drivers/rgb_led.h"

static const char *LOG_TAG = "slam_rover";

void app_main(void) {
  // Initialize RGB led, which is on ESP32-S3 board connected to pin 48
  esp_err_t led_err = init_rgb_led(48);

  // Set color to orange while booting up
  if (led_err == ESP_OK) {
    led_err = set_rgb_led_color(255, 165, 0);
  } else {
    ESP_LOGE(LOG_TAG, "RGB LED init failed: %s", esp_err_to_name(led_err));
  }

  vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay 5 seconds to show led color  change

  // Set color to green once everything is running
  if (led_err == ESP_OK) {
    set_rgb_led_color(0, 255, 0);
  }

  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second
  }
}
