#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "cJSON.h"

#define TAG "ws_server"

// === WebSocket Handler ===
static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake completed with client");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = NULL;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK || ws_pkt.len > 1024) return ESP_FAIL;

    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (!ws_pkt.payload) return ESP_ERR_NO_MEM;

    httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    ws_pkt.payload[ws_pkt.len] = 0;

    ESP_LOGI(TAG, "Received: %s", (char *)ws_pkt.payload);

    cJSON *json = cJSON_Parse((char *)ws_pkt.payload);
    if (json) {
        cJSON *pinItem = cJSON_GetObjectItem(json, "pin");
        cJSON *stateItem = cJSON_GetObjectItem(json, "state");

        if (cJSON_IsNumber(pinItem) && cJSON_IsNumber(stateItem)) {
            int pin = pinItem->valueint;
            int state = stateItem->valueint;

            esp_rom_gpio_pad_select_gpio(pin);
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
            gpio_set_level(pin, state);

            ESP_LOGI(TAG, "GPIO %d => %d", pin, state);

            const char *response = "{\"status\": \"ok\"}";
            httpd_ws_frame_t resp_pkt = {
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)response,
                .len = strlen(response),
            };
            httpd_ws_send_frame(req, &resp_pkt);
        }
        cJSON_Delete(json);
    }

    free(ws_pkt.payload);
    return ESP_OK;
}

// === Стартира HTTP сървъра и регистрира само WebSocket-а ===
httpd_handle_t start_ws_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler,
            .user_ctx = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(TAG, "WebSocket server started at ws://<ip>/ws");
    }

    return server;
}
