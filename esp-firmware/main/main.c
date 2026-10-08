#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "drivers/camera.h"
#include "drivers/rgb_led.h"
#include "network/command_server.h"
#include "network/wifi.h"

static const char *LOG_TAG = "slam_rover";

/**
 * If net init fails -> board is useless -> enter failure loop
 */
void net_fail_loop(esp_err_t led_err) {
  while (1) {
    if (led_err == ESP_OK)
      set_rgb_led_color(255, 0, 0);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (led_err == ESP_OK)
      set_rgb_led_color(255, 165, 0);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/**
 * Actual main loop
 */
void app_main(void) {
  // Initialize RGB led, which is on ESP32-S3 board connected to pin 48
  esp_err_t led_err = init_rgb_led(48);

  // Set color to orange while booting up
  if (led_err == ESP_OK) {
    set_rgb_led_color(255, 165, 0);
  } else {
    ESP_LOGE(LOG_TAG, "RGB LED init failed: %s", esp_err_to_name(led_err));
  }

  // Initialize flash and wifi
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_err);

  esp_err_t wifi_err = initialize_wifi();
  if (wifi_err != ESP_OK) {
    net_fail_loop(led_err);
  }

  // Initialize camera module and camera + server tasks
  init_camera_task();
  init_server_task();

  // Set color to green once everything is running
  if (led_err == ESP_OK) {
    set_rgb_led_color(0, 255, 0);
  }

  // Temporary main loop
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second
  }
}
