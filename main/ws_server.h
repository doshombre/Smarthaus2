#ifndef WS_ECHO_SERVER_H
#define WS_ECHO_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_http_server.h"

// Стартира WS-сървъра и връща handle-а му
httpd_handle_t start_ws_server(void);

#ifdef __cplusplus
}
#endif

#endif // WS_ECHO_SERVER_H
