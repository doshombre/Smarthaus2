oid start_web_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP сървър стартиран");
        // Регистрация задължително първо на WebSocket URI
        register_ws_uri(server);
        // Регистрация на login handler
        // Този handler ще обработва POST заявки към /login
        register_login_handlers(server);
        register_logout_handlers(server);
        // Регистрация на файловия handler
        register_file_server(server);


    } else {
        ESP_LOGE(TAG, "Грешка при старт на сървъра");
    }
}
