#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"

// Старт и стоп на основния HTTP сървър
void start_web_server(void);
void stop_web_server(void);

// (По желание) достъп до текущия httpd_handle_t
httpd_handle_t get_http_server_handle(void);

#endif // WEB_SERVER_H
