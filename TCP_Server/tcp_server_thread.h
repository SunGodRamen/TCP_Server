#ifndef TCP_SERVER_THREAD_H
#define TCP_SERVER_THREAD_H

#include "tcp_server.h"
#include "request_handler.h"
#include "message_protocol.h"
#include "logger.h"
#include <stdbool.h>
#include <windows.h>

typedef struct {
    tcp_socket_info* server_config;
} server_thread_config;

DWORD WINAPI tcp_server_thread(LPVOID port);

#endif // !define TCP_SERVER_THREAD_H
