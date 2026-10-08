#include "command_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *LOG_TAG = "command_server";
static bool server_ready = false;
static int server_fd = -1;

esp_err_t server_start(void) { return ESP_OK; }

bool server_get_client_ip(uint32_t *client_ip) {
  if (!server_ready)
    return false;

  return false;
}

static void server_task(void *arg) {
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

esp_err_t init_server_task(void) {
  if (xTaskCreate(server_task, "server_task", 4096, NULL, (tskIDLE_PRIORITY + 2), NULL) != pdPASS) {
  }

  server_ready = true;
  return ESP_OK;
}
