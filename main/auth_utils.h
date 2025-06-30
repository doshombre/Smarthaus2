#pragma once

#include "esp_http_server.h"

#define MAX_SESSIONS 10

typedef struct {
    char session_id[24];
    char username[32];
    char role[16];
} session_entry_t;

void add_session(const char *sid, const char *user, const char *role);
const char* get_session_id_from_cookie(httpd_req_t *req);
const char* get_role_from_session(const char *sid);
bool has_role(httpd_req_t *req, const char *required_role);
bool is_session_valid(httpd_req_t *req);
const char* get_username_from_session(const char *sid);
const char* get_username(httpd_req_t *req);
bool is_admin(httpd_req_t *req);
