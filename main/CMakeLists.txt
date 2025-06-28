set(srcs "main.c" "ws_echo_server.c" "file_server.c" INCLUDE_DIRS ".")
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs FLASH_IN_PROJECT)


