#pragma once

#include "esp_check.h"

#define MAX_CAMERA_PAYLOAD_SIZE 1024

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t msg_type;
  uint32_t frame_id;
  uint16_t chunk_idx;
  uint16_t chunk_cnt;
} camera_packet_t;

esp_err_t init_camera(void);
esp_err_t init_camera_task(void);