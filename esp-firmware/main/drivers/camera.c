#include "camera.h"
#include "../network/command_server.h"
#include "../network/wire.h"

#include <errno.h>

#include "esp_camera.h"
#include "lwip/sockets.h"

static const char *LOG_TAG = "camera_driver";
static int camera_fd = -1;
static uint8_t camera_chunk[sizeof(camera_packet_t) + MAX_CAMERA_PAYLOAD_SIZE];

esp_err_t init_camera(void) {
  camera_config_t configuration = {
      .pin_pwdn = -1,
      .pin_reset = -1,
      .pin_xclk = 15,
      .pin_sccb_sda = 4,
      .pin_sccb_scl = 5,
      .pin_d7 = 16,
      .pin_d6 = 17,
      .pin_d5 = 18,
      .pin_d4 = 12,
      .pin_d3 = 10,
      .pin_d2 = 8,
      .pin_d1 = 9,
      .pin_d0 = 11,
      .pin_vsync = 6,
      .pin_href = 7,
      .pin_pclk = 13,
      .xclk_freq_hz = 20000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_1,
      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_QVGA, // 320x240
      .jpeg_quality = 12,           // 0 = best .. 63 = worst
      .fb_count = 2,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  };

  return esp_camera_init(&configuration);
}

/**
 * Send frame as chunks over UDP pipe
 */
static void send_frame(const struct sockaddr_in *dest, uint32_t frame_id, const uint8_t *jpeg, size_t jpeg_len) {
  uint16_t chunk_cnt = (uint16_t)((jpeg_len + MAX_CAMERA_PAYLOAD_SIZE - 1) / MAX_CAMERA_PAYLOAD_SIZE);

  // Create header at the beginning of chunk packet
  uint8_t *chunk_ptr = camera_chunk;
  camera_packet_t *header = (camera_packet_t *)chunk_ptr;
  header->magic = WIRE_MAGIC;
  header->msg_type = CAMERA_PACKET;
  header->frame_id = frame_id;
  header->chunk_cnt = chunk_cnt;

  // Transmit chunks
  for (uint16_t i = 0; i < chunk_cnt; i++) {
    size_t src_offset = (size_t)i * MAX_CAMERA_PAYLOAD_SIZE;
    size_t data_remaining = jpeg_len - src_offset;
    size_t packet_len = data_remaining < MAX_CAMERA_PAYLOAD_SIZE ? data_remaining : MAX_CAMERA_PAYLOAD_SIZE;

    header->chunk_idx = i;

    // Copy the chunk data from src jpg to send buffer (after headers)
    memcpy(chunk_ptr + sizeof(camera_packet_t), jpeg + src_offset, packet_len);

    // Send packet
    if (sendto(camera_fd, chunk_ptr, sizeof(camera_packet_t) + packet_len, 0, (const struct sockaddr *)dest,
               sizeof(*dest)) < 0) {
      // Send failed, ignore rest of the chunks
      return;
    }
  }
}

static void camera_task(void *arg) {
  uint32_t frame_id = 0;

  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Method returns false, if nothing is connected
    uint32_t client_ip;
    if (!server_get_client_ip(&client_ip)) {
      continue;
    }

    // If frame buffer pointer is not valid, or jpg len = 0 => nothing to send
    camera_fb_t *cam_fb = esp_camera_fb_get();
    if (!cam_fb) {
      continue;
    }

    if (cam_fb->len == 0) {
      esp_camera_fb_return(cam_fb);
      continue;
    }

    // Transmit frame, to client .. UDP port 5001
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port = htons(5001),
        .sin_addr.s_addr = client_ip,
    };

    send_frame(&dest, frame_id, cam_fb->buf, cam_fb->len);

    frame_id++;
    esp_camera_fb_return(cam_fb);
  }
}

esp_err_t init_camera_task(void) {
  // Initialize camera module
  esp_err_t cam_err = init_camera();
  if (cam_err != ESP_OK) {
    ESP_LOGE(LOG_TAG, "Camera initialization failed: %s", esp_err_to_name(cam_err));
    return cam_err;
  }

  // Initialize UDP socket for sending frames
  camera_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (camera_fd < 0) {
    ESP_LOGE(LOG_TAG, "Camera UDP socket creation failed, errno: %d", errno);

    esp_camera_deinit();
    return ESP_FAIL;
  }

  // Start camera frame sender task
  if (xTaskCreate(camera_task, "camera_task", 4096, NULL, (tskIDLE_PRIORITY + 1), NULL) != pdPASS) {
    ESP_LOGE(LOG_TAG, "Camera task creation failed.. out of mem?");

    close(camera_fd);
    camera_fd = -1;

    esp_camera_deinit();
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}