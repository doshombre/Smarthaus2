#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "file_server.h"

static const char *TAG = "file_server";

static esp_err_t file_get_handler(httpd_req_t *req) {
    char filepath[256];
    const char *base_path = "/spiffs";

    if (strcmp(req->uri, "/") == 0) {
        // Когато е root, serve /index.html
        strlcpy(filepath, base_path, sizeof(filepath));
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcpy(filepath, base_path, sizeof(filepath));
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Not found: %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // Задаване на Content-Type според разширението
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
