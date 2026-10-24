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
 * If net/server/camera init fails -> board is useless -> enter failure loop
 */
static void fail_loop(esp_err_t led_err, bool networkErr) {
  while (1) {
    if (led_err == ESP_OK)
      set_rgb_led_color(networkErr ? 255 : 0, 0, networkErr ? 0 : 255);

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

  // Initialize flash, wifi and server task
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_err);

  if (initialize_wifi() != ESP_OK) {
    fail_loop(led_err, true);
  }

  if (init_server_task() != ESP_OK) {
    fail_loop(led_err, true);
  }

  // Initialize camera module and camera task
  if (init_camera_task() != ESP_OK) {
    fail_loop(led_err, false);
  }

  // Set color to green once everything is running
  if (led_err == ESP_OK) {
    set_rgb_led_color(0, 255, 0);
  }

  // Note: even though main method returns, the tasks (at least command server) keep the firmware running
}
