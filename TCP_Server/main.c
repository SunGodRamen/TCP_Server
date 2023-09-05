#include "config.h"
#include "tcp_server_thread.h"
#include "logger.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_LEVEL _DEBUG
#define SUCCESS 1
#define FAILURE 0
#define THREAD_START_ROUTINE tcp_server_thread
#define THREAD_CREATION_FLAGS 0

// Forward declarations
int create_threads(HANDLE* tcp_threads, server_thread_config** thread_configs);
void cleanup_resources(HANDLE* tcp_threads, server_thread_config** thread_configs, int count);

int main() {
    init_logger(LOG_FILE);
    set_log_level(LOG_LEVEL);
    write_log(_INFO, "Main - Application started");

    HANDLE tcp_threads[NUM_PORTS];
    server_thread_config* thread_configs[NUM_PORTS];

    if (!create_threads(tcp_threads, thread_configs)) {
        cleanup_resources(tcp_threads, thread_configs, NUM_PORTS);
        write_log(_ERROR, "Main - Failed to create threads and initialize configs");
        return FAILURE;
    }

    for (int i = 0; i < NUM_PORTS; i++) {
        WaitForSingleObject(tcp_threads[i], INFINITE);
        CloseHandle(tcp_threads[i]);
    }

    cleanup_resources(tcp_threads, thread_configs, NUM_PORTS);
    write_log(_INFO, "Main - Cleanup completed");
    close_logger();

    return SUCCESS;
}

int create_threads(HANDLE* tcp_threads, server_thread_config** thread_configs) {
    for (int i = 0; i < NUM_PORTS; ++i) {
        tcp_socket_info* server_info_ptr = malloc(sizeof(tcp_socket_info));
        if (!server_info_ptr) {
            write_log(_ERROR, "Main - Error allocating memory for server_info");
            return FAILURE;
        }

        server_info_ptr->ip = "127.0.0.1";  // Move to config?
        server_info_ptr->port = TCP_PORTS[i];

        server_thread_config* server_thread_config_ptr = malloc(sizeof(server_thread_config));
        if (!server_thread_config_ptr) {
            write_log(_ERROR, "Main - Error allocating memory for server_thread_config");
            free(server_info_ptr);
            return FAILURE;
        }

        server_thread_config_ptr->server_config = server_info_ptr;
        thread_configs[i] = server_thread_config_ptr;

        tcp_threads[i] = CreateThread(NULL, 0, THREAD_START_ROUTINE, thread_configs[i], THREAD_CREATION_FLAGS, NULL);
        if (tcp_threads[i] == NULL) {
            write_log(_ERROR, "Main - Error creating thread for a port");
            free(server_thread_config_ptr->server_config);
            free(server_thread_config_ptr);
            return FAILURE;
        }
    }

    write_log(_INFO, "Main - Threads and configurations successfully created");
    return SUCCESS;
}

void cleanup_resources(HANDLE* tcp_threads, server_thread_config** thread_configs, int count) {
    for (int i = 0; i < count; i++) {
        if (thread_configs[i]) {
            free(thread_configs[i]->server_config);
            free(thread_configs[i]);
        }
    }
}
