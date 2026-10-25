#include <string.h>

#include "command_server.h"
#include "wire.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"

static const char *LOG_TAG = "command_server";
static int server_tcp_fd = -1;
static int server_udp_fd = -1;
static uint32_t client_ip = 0;

bool server_get_client_ip(uint32_t *client_ip_ptr) {
  if (client_ip == 0)
    return false;

  *client_ip_ptr = client_ip;
  return true;
}

/**
 * Handle possible UDP command packet + validate size and source
 */
static void handle_udp_packet(uint8_t *buf, size_t buf_len) {
  struct sockaddr_in src;
  socklen_t src_len = sizeof(src);
  int cnt = recvfrom(server_udp_fd, buf, buf_len, 0, (struct sockaddr *)&src, &src_len);

  // Packet size or sender IP doesn't match expected
  if (cnt != sizeof(udp_command_packet_t) || src.sin_addr.s_addr != client_ip)
    return;

  // Check that magic matches
  udp_command_packet_t udp_packet;
  memcpy(&udp_packet, buf, sizeof(udp_packet));
  if (udp_packet.magic != WIRE_MAGIC)
    return;
}

/**
 * Loop that handles the client events, after connection is established
 */
static void client_loop(int client_fd) {
  int max_fd = (client_fd > server_udp_fd ? client_fd : server_udp_fd) + 1;

  // TCP doesn't really send anything, all commands are sent via UDP
  uint8_t buf[sizeof(udp_command_packet_t) + 1];

  // Drain old UDP packets
  while (recv(server_udp_fd, buf, sizeof(buf), MSG_DONTWAIT) > 0) {
  }

  // Start processing packets
  while (1) {

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(client_fd, &fds);
    FD_SET(server_udp_fd, &fds);

    if (select(max_fd, &fds, NULL, NULL, NULL) < 0) {
      ESP_LOGE(LOG_TAG, "select failed, errno %d", errno);
      break;
    }

    // Process TCP packet, if one has been received
    if (FD_ISSET(client_fd, &fds)) {
      int received = recv(client_fd, buf, sizeof(buf), 0);
      if (received == 0) {
        ESP_LOGI(LOG_TAG, "Client disconnected");
        break;
      }
      if (received < 0) {
        ESP_LOGE(LOG_TAG, "recv failed, errno %d", errno);
        break;
      }
    }

    // Process UDP packet, if one has been received from TCP clients address
    if (FD_ISSET(server_udp_fd, &fds)) {
      handle_udp_packet(buf, sizeof(buf));
    }
  }

  // Client disconnected, close socket and remove ip
  close(client_fd);
  client_ip = 0;
}

/**
 * The actual task, that accepts the connection and handles client messages
 */
static void server_task(void *arg) {
  while (1) {
    // Wait for client to connect
    ESP_LOGI(LOG_TAG, "Waiting for connection...");
    struct sockaddr_in client_addr;
    socklen_t peer_len = sizeof(client_addr);
    int client_fd = accept(server_tcp_fd, (struct sockaddr *)&client_addr, &peer_len);
    if (client_fd < 0) {
      ESP_LOGE(LOG_TAG, "accept failed, errno %d", errno);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGI(LOG_TAG, "Client connected from '%s'", inet_ntoa(client_addr.sin_addr));
    client_ip = client_addr.sin_addr.s_addr;

    // Enable keepalive polling to client socket
    int keepalive_on = 1, idle = 5, interval = 2, count = 3;
    setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive_on, sizeof(keepalive_on));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));

    // Handle client connection and disconnection
    client_loop(client_fd);
  }
}

static esp_err_t create_listening_socket(int *socket_fd, int port, bool is_tcp) {
  *socket_fd = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, is_tcp ? IPPROTO_TCP : IPPROTO_UDP);
  if (*socket_fd < 0) {
    ESP_LOGE(LOG_TAG, "socket failed, errno %d", errno);
    return ESP_FAIL;
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(INADDR_ANY),
      .sin_port = htons(port),
  };
  if (bind(*socket_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    ESP_LOGE(LOG_TAG, "bind failed, errno %d", errno);
    close(*socket_fd);
    *socket_fd = -1;
    return ESP_FAIL;
  }

  // UDP socket does not need listen
  if (!is_tcp) {
    return ESP_OK;
  }

  if (listen(*socket_fd, 1) != 0) {
    ESP_LOGE(LOG_TAG, "listen failed, errno %d", errno);
    close(*socket_fd);
    *socket_fd = -1;
    return ESP_FAIL;
  }

  return ESP_OK;
}

/**
 * Create listening socket and start server task, which accepts new connections
 */
esp_err_t init_server_task(void) {
  // Create listening sockets (TCP and UDP)
  esp_err_t tcp_err = create_listening_socket(&server_tcp_fd, 5000, true);
  if (tcp_err != ESP_OK) {
    return ESP_FAIL;
  }

  esp_err_t udp_err = create_listening_socket(&server_udp_fd, 5002, false);
  if (udp_err != ESP_OK) {
    close(server_tcp_fd);
    server_tcp_fd = -1;
    return ESP_FAIL;
  }

  // Create listening task
  if (xTaskCreate(server_task, "server_task", 4096, NULL, (tskIDLE_PRIORITY + 2), NULL) != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create server task, out of mem?");
    close(server_tcp_fd);
    close(server_udp_fd);
    server_tcp_fd = -1;
    server_udp_fd = -1;
    return ESP_FAIL;
  }

  return ESP_OK;
}
