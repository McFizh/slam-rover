#pragma once

#define WIRE_MAGIC 0xC4
#define CAMERA_PACKET 1

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t seq;
} udp_command_packet_t;

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t msg_type;
  uint32_t frame_id;
  uint16_t chunk_idx;
  uint16_t chunk_cnt;
} camera_packet_t;