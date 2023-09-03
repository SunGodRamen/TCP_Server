// TCP_Server.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>
#include "logger.h"

typedef struct {
	const char* ip;  // IP address of the server
	uint16_t port;   // Port number to connect to
} tcp_socket_info;

SOCKET init_server(tcp_socket_info* socket_info);
int read_message_from_client(SOCKET clientSocket, char* buffer);
SOCKET accept_connection(SOCKET serverSocket);
int receive_from_client(SOCKET clientSocket, char* buffer, int bufferSize);
void send_to_client(SOCKET clientSocket, const char* response, int responseLength);
void cleanup_server(SOCKET serverSocket, SOCKET clientSocket);

#endif
