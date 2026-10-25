#include "rgb_led.h"

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

static const char *LOG_TAG = "rgb_led_driver";

rmt_channel_handle_t rmt_channel;
rmt_encoder_handle_t rmt_encoder;

/**
 * Initialize RMT peripheral to drive WS2812B RGB led
 */
esp_err_t init_rgb_led(int gpio_pin) {
  // Use remote control transmitter peripheral to drive the rgb led
  rmt_tx_channel_config_t channel_configuration = {.clk_src = RMT_CLK_SRC_DEFAULT,
                                                   .resolution_hz = 10000000, // 10MHz, 1 tick = 100ns
                                                   .mem_block_symbols = 64,
                                                   .trans_queue_depth = 4,
                                                   .gpio_num = gpio_pin};

  ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_configuration, &rmt_channel), LOG_TAG,
                      "RMT TX Channel configuration");

  // WS2812B bit timings
  rmt_bytes_encoder_config_t encoder_configuration = {
      .bit0 =
          {
              .level0 = 1,
              .duration0 = 3, // 3 ticks  = 300ns
              .level1 = 0,
              .duration1 = 9, // 9 ticks = 900ns
          },
      .bit1 =
          {
              .level0 = 1,
              .duration0 = 9, // 9 ticks = 900ns
              .level1 = 0,
              .duration1 = 3, // 3 ticks = 300ns
          },
      .flags.msb_first = 1,
  };

  ESP_RETURN_ON_ERROR(rmt_new_bytes_encoder(&encoder_configuration, &rmt_encoder), LOG_TAG, "RMT new bytes encoder");
  ESP_RETURN_ON_ERROR(rmt_enable(rmt_channel), LOG_TAG, "RMT enable");

  return ESP_OK;
}

esp_err_t set_rgb_led_color(uint8_t r, uint8_t g, uint8_t b) {
  // Note: Data format for WS2812B is: green, red, blue
  uint8_t rgb_data[3] = {g, r, b};
  rmt_transmit_config_t tx_configuration = {
      .loop_count = 0,
  };

  ESP_RETURN_ON_ERROR(rmt_transmit(rmt_channel, rmt_encoder, rgb_data, sizeof(rgb_data), &tx_configuration), LOG_TAG,
                      "RMT transmit");
  ESP_RETURN_ON_ERROR(rmt_tx_wait_all_done(rmt_channel, 1000), LOG_TAG, "RMT wait for transmit");

  return ESP_OK;
}