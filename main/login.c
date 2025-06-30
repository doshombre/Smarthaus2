#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_random.h"

#include "mbedtls/sha1.h"
#include "cJSON.h"
#include "login.h"
#include "auth_utils.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static const char *TAG = "login";



static void sha1_hash(const char *input, char *output_hex) {
    unsigned char digest[20];  // SHA-1 = 160 бита
    mbedtls_sha1((const unsigned char *)input, strlen(input), digest);
    for (int i = 0; i < 20; ++i) {
        sprintf(&output_hex[i * 2], "%02x", digest[i]);
    }
}


static esp_err_t login_handler(httpd_req_t *req) {
    int total_len = req->content_len;
    char content[256] = {0};
    int ret = httpd_req_recv(req, content, MIN(sizeof(content) - 1, total_len));
    if (ret <= 0) return ESP_FAIL;

    // Очакваме JSON { "username": "...", "password": "..." }
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        ESP_LOGW(TAG, "Невалиден JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *user_item = cJSON_GetObjectItem(root, "username");
    const cJSON *pass_item = cJSON_GetObjectItem(root, "password");
    if (!cJSON_IsString(user_item) || !cJSON_IsString(pass_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing credentials");
        return ESP_FAIL;
    }

    const char *username = user_item->valuestring;
    const char *password = pass_item->valuestring;

    // Хеширане на паролата с SHA-1
    char password_sha1[41];  // 20 байта → 40 hex + \0
    sha1_hash(password, password_sha1);

    // Четене на users.json
    FILE *file = fopen("/spiffs/users.json", "r");
    if (!file) {
        ESP_LOGE(TAG, "Грешка при отваряне на users.json");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    char buffer[1024] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);

    cJSON *users = cJSON_Parse(buffer);
    if (!users || !cJSON_IsArray(users)) {
        ESP_LOGE(TAG, "users.json не е валиден JSON масив");
        cJSON_Delete(root);
        cJSON_Delete(users);
        return ESP_FAIL;
    }

    bool success = false;

    cJSON *user = NULL;
    cJSON_ArrayForEach(user, users) {
        const cJSON *user_name = cJSON_GetObjectItem(user, "user");
        const cJSON *user_pass = cJSON_GetObjectItem(user, "pass");
        const cJSON *role_item = cJSON_GetObjectItem(user, "role");
        const char *role = (cJSON_IsString(role_item)) ? role_item->valuestring : "user";

        if (cJSON_IsString(user_name) && cJSON_IsString(user_pass)) {
            if (strcmp(user_name->valuestring, username) == 0 &&
                strcmp(user_pass->valuestring, password_sha1) == 0) {

                success = true;

                // Генерираме session ID
                uint32_t sid = esp_random() % 99999999;
                char session_id[24];
                snprintf(session_id, sizeof(session_id), "sess%08lX", sid);
                add_session(session_id, username, role);
                char cookie_header[64];
                snprintf(cookie_header, sizeof(cookie_header),
                         "session=%s; Path=/; HttpOnly", session_id);
                httpd_resp_set_hdr(req, "Set-Cookie", cookie_header);

                // Redirect
                httpd_resp_set_status(req, "302 Found");
                httpd_resp_set_hdr(req, "Location", "/index.html");
                httpd_resp_send(req, NULL, 0);
                break;
            }
        }
    }

    if (!success) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Грешно потребителско име или парола\"}");
    }

    cJSON_Delete(root);
    cJSON_Delete(users);
    return ESP_OK;
}

void register_login_handlers(httpd_handle_t server) {
    httpd_uri_t login_post = {
        .uri = "/login",
        .method = HTTP_POST,
        .handler = login_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &login_post);
}
esp_err_t logout_handler(httpd_req_t *req) {
    ESP_LOGI("LOGOUT", "Извършване на logout");

    httpd_resp_set_hdr(req, "Set-Cookie", "session=; Path=/; Max-Age=0; HttpOnly");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/login.html?msg=Успешен%20изход");
    return httpd_resp_send(req, NULL, 0);

}
void register_logout_handlers(httpd_handle_t server) {
    httpd_uri_t logout = {
        .uri = "/logout",
        .method = HTTP_GET,
        .handler = logout_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &logout);
}


