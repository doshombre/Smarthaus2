#pragma once
#include "esp_http_server.h"

void register_login_handlers(httpd_handle_t server);
void register_logout_handlers(httpd_handle_t server);
