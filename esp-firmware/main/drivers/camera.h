#pragma once

#include "esp_check.h"

// Camera image is split into chunks of this many bytes (+ header). Keeping total size below
// 1400 bytes, should make sure that network layer doesn't have to fragment udp packages
#define MAX_CAMERA_PAYLOAD_SIZE 1300

esp_err_t init_camera(void);
esp_err_t init_camera_task(void);
