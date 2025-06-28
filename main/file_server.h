#pragma once
#include "esp_http_server.h"

// Регистрира файловия handler към подаден сървър
void register_file_server(httpd_handle_t server);
