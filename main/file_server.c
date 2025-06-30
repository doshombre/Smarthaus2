#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "file_server.h"
#include <stdlib.h>     // за malloc/free
#include <string.h>     // за strstr, strcmp
#include "auth_utils.h"

static const char *TAG = "file_server";
// 🔧 Strip на query string (напр. ?msg=...)
static void strip_query(char *dst, const char *uri, size_t max_len) {
    strlcpy(dst, uri, max_len);
    char *q = strchr(dst, '?');
    if (q) *q = '\0';
}

// 🪵 Функция за логване на всички заглавки (headers)
static void log_all_headers(httpd_req_t *req) {
    const char *headers[] = {
        "Host", "User-Agent", "Accept", "Accept-Encoding",
        "Accept-Language", "Referer", "Origin", "Cookie",
        "Connection", "Upgrade-Insecure-Requests", "Content-Type"
    };

    char value[256];
    for (size_t i = 0; i < sizeof(headers)/sizeof(headers[0]); ++i) {
        if (httpd_req_get_hdr_value_str(req, headers[i], value, sizeof(value)) == ESP_OK) {
            ESP_LOGI("HEADER", "%s: %s", headers[i], value);
        }
    }
}
esp_err_t file_get_handler(httpd_req_t *req) {
    char clean_uri[128];
    strip_query(clean_uri, req->uri, sizeof(clean_uri));

    ESP_LOGI(TAG, "Заявка: %s", clean_uri);
    log_all_headers(req);  // 🪵 Логваме всички заглавки

    // Изключения от сесия проверка
    if (strcmp(clean_uri, "/login.html") != 0 &&
        strcmp(clean_uri, "/login") != 0 &&
        strcmp(clean_uri, "/logout") != 0 &&
        strstr(clean_uri, ".css") == NULL &&
        strstr(clean_uri, ".js") == NULL &&
        strstr(clean_uri, ".ico") == NULL &&
        strstr(clean_uri, ".png") == NULL &&
        strstr(clean_uri, ".jpg") == NULL &&
        strstr(clean_uri, ".jpeg") == NULL) {

        if (!is_session_valid(req)) {
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/login.html?msg=Изисква%20вход");
            httpd_resp_send(req, NULL, 0);
            return ESP_OK;
        }
    }

    // Път до файла
    char filepath[256];
    const char *base_path = "/spiffs";

    if (strcmp(clean_uri, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", base_path);
    } else {
        snprintf(filepath, sizeof(filepath), "%s%s", base_path, clean_uri);
    }

    ESP_LOGI(TAG, "Файл: %s", filepath);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Файлът не съществува: %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Файлът не е намерен");
        return ESP_FAIL;
    }

    // Content-Type
    if (strstr(filepath, ".html")) httpd_resp_set_type(req, "text/html");
    else if (strstr(filepath, ".css")) httpd_resp_set_type(req, "text/css");
    else if (strstr(filepath, ".js")) httpd_resp_set_type(req, "application/javascript");
    else if (strstr(filepath, ".png")) httpd_resp_set_type(req, "image/png");
    else if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) httpd_resp_set_type(req, "image/jpeg");
    else httpd_resp_set_type(req, "text/plain");

    char buffer[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
void register_file_server(httpd_handle_t server) {
    httpd_uri_t file_handler = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = file_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &file_handler);
    ESP_LOGI(TAG, "File server handler registered");
}
