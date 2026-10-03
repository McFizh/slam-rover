#pragma once

#include "esp_check.h"

esp_err_t init_rgb_led(int gpio_pin);
esp_err_t set_rgb_led_color(uint8_t r, uint8_t g, uint8_t b);