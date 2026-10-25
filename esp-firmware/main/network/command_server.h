#pragma once

#include <stdbool.h>

#include "esp_check.h"

bool server_get_client_ip(uint32_t *client_ip);
esp_err_t init_server_task(void);
