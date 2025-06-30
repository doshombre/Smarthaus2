set(srcs "main.c" "ws_server.c" "web_server.c" "file_server.c"
 "login.c" auth_utils.c INCLUDE_DIRS ".")
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs FLASH_IN_PROJECT)


