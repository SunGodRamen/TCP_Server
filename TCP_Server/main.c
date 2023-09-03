#include "config.h"
#include "tcp_server_thread.h"
#include "logger.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_LEVEL _DEBUG

int create_threads(HANDLE* tcp_threads, server_thread_config** thread_configs);

int main() {
    // Logging application start
    write_log(_INFO, "Main - Application started");

    // Initialization code
    init_logger(LOG_FILE);
    set_log_level(LOG_LEVEL);
    write_log(_INFO, "Main - Logger initialized");

    // Create threads and initialize configs
    HANDLE tcp_threads[NUM_PORTS];
    server_thread_config* thread_configs[NUM_PORTS];
    if (!create_threads(tcp_threads, thread_configs)) {
        write_log(_ERROR, "Main - Failed to create threads and initialize configs");
        return 1;
    }

    // Wait for threads to complete
    for (int i = 0; i < NUM_PORTS; i++) {
        WaitForSingleObject(tcp_threads[i], INFINITE);
        CloseHandle(tcp_threads[i]);
    }

    // Cleanup
    for (int i = 0; i < NUM_PORTS; i++) {
        free(thread_configs[i]->server_config);
        free(thread_configs[i]);
    }

    write_log(_INFO, "Main - Cleanup completed");

    // Close logger
    close_logger();

    return 0;
}

int create_threads(HANDLE* tcp_threads, server_thread_config** thread_configs) {
    for (int i = 0; i < NUM_PORTS; ++i) {
        // Initialize server configuration
        tcp_socket_info* server_info_ptr = (tcp_socket_info*)malloc(sizeof(tcp_socket_info));
        if (server_info_ptr == NULL) {
            write_log(_ERROR, "Main - Error allocating memory for server_info");
            return 0;
        }

        server_info_ptr->ip = "127.0.0.1";
        server_info_ptr->port = TCP_PORTS[i];

        server_thread_config* server_thread_config_ptr = (server_thread_config*)malloc(sizeof(server_thread_config));
        if (server_thread_config_ptr == NULL) {
            write_log(_ERROR, "Main - Error allocating memory for server_thread_config");
            free(server_info_ptr);
            return 0;
        }

        server_thread_config_ptr->server_config = server_info_ptr;
        thread_configs[i] = server_thread_config_ptr;

        // Create thread
        tcp_threads[i] = CreateThread(NULL, 0, tcp_server_thread, thread_configs[i], 0, NULL);
        if (tcp_threads[i] == NULL) {
            write_log(_ERROR, "Main - Error creating thread for a port");
            free(server_thread_config_ptr->server_config);
            free(server_thread_config_ptr);
            return 0;
        }
    }

    write_log(_INFO, "Main - Threads and configurations successfully created");
    return 1;
}
