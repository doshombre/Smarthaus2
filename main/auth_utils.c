#include <string.h>
#include "auth_utils.h"
#include "esp_log.h"
static const char *TAG = "auth_utils";
#define MAX_SESSIONS 10

session_entry_t sessions[MAX_SESSIONS] = {0};

// 📥 Извлича session ID от Cookie header
const char* get_session_id_from_cookie(httpd_req_t *req) {
    static char sid[32];  // static е важно — иначе връща невалиден указател
    char cookie[128] = {0};

    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) == ESP_OK) {
        char *start = strstr(cookie, "session=");
        if (start) {
            start += strlen("session=");
            char *end = strchr(start, ';');
            size_t len = end ? (size_t)(end - start) : strlen(start);
            strncpy(sid, start, len);
            sid[len] = '\0';
            return sid;
        }
    }
    return NULL;
}

// 🧾 Връща роля от запазените сесии
const char* get_role_from_session(const char *sid) {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (strcmp(sessions[i].session_id, sid) == 0) {
            return sessions[i].role;
        }
    }
    return "guest"; // по подразбиране
}

// 📌 Добавя нова сесия
void add_session(const char *sid, const char *user, const char *role) {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (sessions[i].session_id[0] == '\0') {
            strncpy(sessions[i].session_id, sid, sizeof(sessions[i].session_id));
            strncpy(sessions[i].username, user, sizeof(sessions[i].username));
            strncpy(sessions[i].role, role, sizeof(sessions[i].role));
            break;
        }
    }
}

// ✅ Проверява дали текущата сесия има определена роля
bool has_role(httpd_req_t *req, const char *required) {
    const char *sid = get_session_id_from_cookie(req);
    const char *role = sid ? get_role_from_session(sid) : "guest";
    return strcmp(role, required) == 0;
}

// ✅ Проверка за session cookie
bool is_session_valid(httpd_req_t *req) {
    size_t cookie_len = httpd_req_get_hdr_value_len(req, "Cookie") + 1;
    if (cookie_len > 1) {
        char *cookie = malloc(cookie_len);
        if (cookie && httpd_req_get_hdr_value_str(req, "Cookie", cookie, cookie_len) == ESP_OK) {
            bool valid = strstr(cookie, "session=") != NULL;
            ESP_LOGI(TAG, "Cookie: %s [%s]", cookie, valid ? "VALID" : "MISSING session=");
            free(cookie);
            return valid;
        }
        if (cookie) free(cookie);
    }
    return false;
}


const char* get_username_from_session(const char *sid) {
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (strcmp(sessions[i].session_id, sid) == 0) {
            return sessions[i].username;
        }
    }
    return "guest";
}

const char* get_username(httpd_req_t *req) {
    const char *sid = get_session_id_from_cookie(req);
    return sid ? get_username_from_session(sid) : "guest";
}

bool is_admin(httpd_req_t *req) {
    const char *sid = get_session_id_from_cookie(req);
    const char *role = sid ? get_role_from_session(sid) : "guest";
    return strcmp(role, "admin") == 0;
}
