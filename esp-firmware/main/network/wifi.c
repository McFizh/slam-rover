#include "wifi.h"

#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "mdns.h"

#define CONNECTED_BIT BIT0
#define FAIL_BIT BIT1

static const char *LOG_TAG = "wifi";
static int retry_num = 0;

static EventGroupHandle_t wifi_event_group;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGI(LOG_TAG, "connecting to AP failed, retry: %d", retry_num);
    if (retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      retry_num++;
    } else {
      xEventGroupSetBits(wifi_event_group, FAIL_BIT);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(LOG_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    retry_num = 0;
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
  }
}

esp_err_t initialize_mdns(void) {
  esp_err_t err = mdns_init();
  if (err != ESP_OK) {
    return err;
  }
  err = mdns_hostname_set("slamrover");
  if (err != ESP_OK) {
    return err;
  }
  mdns_instance_name_set("SLAM rover");
  return ESP_OK;
}

esp_err_t initialize_wifi(void) {
  wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init_config));

  // Register event handlers
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  // Configure ssid and password
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_ESP_WIFI_SSID,
              .password = CONFIG_ESP_WIFI_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(LOG_TAG, "wifi init finished");

  // Wait for connection or failure
  EventBits_t eventBits =
      xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  if (eventBits & CONNECTED_BIT) {
    // Start mdns, if connected
    esp_err_t mdns_err = initialize_mdns();
    if (mdns_err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "mDNS initialization failed: %s", esp_err_to_name(mdns_err));
    } else {
      ESP_LOGI(LOG_TAG, "mDNS initialized");
    }
    return ESP_OK;
  } else if (eventBits & FAIL_BIT) {
    return ESP_FAIL;
  } else {
    ESP_LOGE(LOG_TAG, "Unexpected event bit set");
    return ESP_FAIL;
  }
}