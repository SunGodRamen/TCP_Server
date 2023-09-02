#include "config.h"
#include "tcp_server_thread.h"
#include "logger.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_LEVEL _DEBUG

server_thread_config* init_server_thread_config(int port);

int main() {
    // Logging application start
    write_log(_INFO, "Application started");

    // Initialization code
    init_logger(LOG_FILE);
    set_log_level(LOG_LEVEL);
    write_log(_INFO, "Logger initialized");

    // Initialize server configurations
    server_thread_config* thread_configs[NUM_PORTS];
    for (int i = 0; i < NUM_PORTS; i++) {
        write_log_format(_INFO, "Creating config for port %d", TCP_PORTS[i]);
        thread_configs[i] = init_server_thread_config(TCP_PORTS[i]);
        if (thread_configs[i] == NULL) {
            write_log(_ERROR, "Failed to initialize server thread config.");
            return 1;
        }
    }

    // Create threads
    HANDLE tcp_threads[NUM_PORTS];
    for (int i = 0; i < NUM_PORTS; i++) {
        write_log_format(_INFO, "Creating thread for port %d", thread_configs[i]->server_config->port);
        tcp_threads[i] = CreateThread(NULL, 0, tcp_server_thread, thread_configs[i], 0, NULL);
        if (tcp_threads[i] == NULL) {
            // Handle thread creation error
            write_log(_ERROR, "Error creating thread for a port.");
            for (int j = 0; j < i; j++) {
                CloseHandle(tcp_threads[j]);
            }
            return 0;
        }
    }
    write_log(_INFO, "Threads created");

    // Wait for threads to complete
    for (int i = 0; i < NUM_PORTS; i++) {
        WaitForSingleObject(tcp_threads[i], INFINITE);
    }

    // Cleanup
    for (int i = 0; i < NUM_PORTS; i++) {
        CloseHandle(tcp_threads[i]);
    }

    // Free allocated memory
    for (int i = 0; i < NUM_PORTS; i++) {
        free(thread_configs[i]->server_config);
        free(thread_configs[i]);
    }

    write_log(_INFO, "Cleanup completed");

    // Close logger
    close_logger();

    return 0;
}

server_thread_config* init_server_thread_config(int port) {

    tcp_socket_info* server_info_ptr = (tcp_socket_info*)malloc(sizeof(tcp_socket_info));
    if (server_info_ptr == NULL) {
        write_log(_ERROR, "Error allocating memory for server_info\n");
        free(server_info_ptr);
        return NULL;
    }
    server_info_ptr->ip = "127.0.0.1";
    server_info_ptr->port = port;

    server_thread_config* server_thread_config_ptr = (server_thread_config*)malloc(sizeof(server_thread_config));
    if (server_thread_config_ptr == NULL) {
        write_log(_ERROR, "Error allocating memory for server_thread_config\n");
        free(server_info_ptr);
        return NULL;
    }
    server_thread_config_ptr->server_config = server_info_ptr;

    // Log the IP and port information
    write_log_format(_INFO, "Initialized server_thread_config with IP: %s, Port: %d", server_info_ptr->ip, server_info_ptr->port);

    return server_thread_config_ptr;
}
