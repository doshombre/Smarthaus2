#ifndef WS_SERVER_H
#define WS_SERVER_H

#include "esp_http_server.h"

void register_ws_uri(httpd_handle_t server);

#endif
