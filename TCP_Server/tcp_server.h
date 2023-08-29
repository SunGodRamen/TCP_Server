// TCP_Server.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>

#include "logger.h"

// Read a 64 bit message from the client
int read_message_from_client(SOCKET clientSocket, char* buffer);

// Initialize the server and start listening
SOCKET init_server(uint16_t port);

// Accept a connection
SOCKET accept_connection(SOCKET serverSocket);

// Receive data from a client
int receive_from_client(SOCKET clientSocket, char* buffer, int bufferSize);

// Send a response to a client
void send_to_client(SOCKET clientSocket, const char* response, int responseLength);

// Cleanup and close the server
void cleanup_server(SOCKET serverSocket, SOCKET clientSocket);

#endif
