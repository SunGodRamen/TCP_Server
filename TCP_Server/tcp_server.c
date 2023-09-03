#include "tcp_server.h"

/**
 * Initializes the server socket and binds it to the port specified in socket_info.
 *
 * @param socket_info A pointer to a tcp_socket_info struct containing port number and other socket options.
 * @return A valid server socket or exits the program if an error occurs.
 */
SOCKET init_server(tcp_socket_info* socket_info) {
    write_log_format(_INFO, "TCP Server - Initializing server on port %d...", socket_info->port);
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        write_log_format(_ERROR, "TCP Server - Failed to initialize WinSock. Error Code: %d", WSAGetLastError());
        exit(1);
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        write_log_format(_ERROR, "TCP Server - Failed to create socket. Error Code: %d", WSAGetLastError());
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(socket_info->port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        write_log_format(_ERROR, "TCP Server - Bind failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        write_log_format(_ERROR, "TCP Server - Listen failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    write_log(_INFO, "TCP Server - Server initialized successfully.");
    return serverSocket;
}

/**
 * Reads a fixed-length message from the client.
 *
 * @param clientSocket The client's socket.
 * @param buffer The buffer to store the message read from the client.
 * @return The number of bytes read, -1 if an error occurs, or 0 if timed out.
 */
int read_message_from_client(SOCKET clientSocket, char* buffer) {
    write_log(_DEBUG, "TCP Server - Starting read_message_from_client.");
    fd_set readSet;
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_usec = 0;

    FD_ZERO(&readSet);
    FD_SET(clientSocket, &readSet);
    int selectResult = select(clientSocket + 1, &readSet, NULL, NULL, &timeout);

    if (selectResult == SOCKET_ERROR) {
        write_log_format(_ERROR, "TCP Server - Select failed. Error Code: %d", WSAGetLastError());
        return -1;
    }
    if (selectResult == 0) {
        write_log(_WARN, "TCP Server - Client read timed out.");
        return 0; // Timeout
    }

    int totalBytesRead = 0;
    int bytesRead = 0;
    int bytesToRead = 8; // 64 bits

    while (totalBytesRead < bytesToRead) {
        bytesRead = recv(clientSocket, buffer + totalBytesRead, bytesToRead - totalBytesRead, 0);
        if (bytesRead == SOCKET_ERROR && WSAGetLastError() != 10053) {
            write_log_format(_ERROR, "TCP Server - Read from client failed. Error Code: %d", WSAGetLastError());
            return -1;
        }
        else if (bytesRead == 0) {
            write_log(_WARN, "TCP Server - Client disconnected before sending full message.");
            return totalBytesRead;
        }
        totalBytesRead += bytesRead;
    }

    write_log_byte_array(_DEBUG, buffer, totalBytesRead);
    write_log_format(_DEBUG, "TCP Server - Received %d bytes from client.", totalBytesRead);

    return totalBytesRead;
}

/**
 * Accepts a new client connection.
 *
 * @param serverSocket The server's socket.
 * @return A new client socket if the connection is successful, otherwise exits the program.
 */
SOCKET accept_connection(SOCKET serverSocket) {
    write_log(_INFO, "TCP Server - Waiting for client connection...");
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        write_log_format(_ERROR, "TCP Server - Accept failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    write_log(_INFO, "TCP Server - Client connected.");
    return clientSocket;
}

/**
 * Receives data from the client.
 *
 * @param clientSocket The client's socket.
 * @param buffer The buffer to store received data.
 * @param bufferSize The size of the buffer in bytes.
 * @return The number of bytes received or -1 if an error occurs.
 */
int receive_from_client(SOCKET clientSocket, char* buffer, int bufferSize) {
    write_log(_DEBUG, "TCP Server - Receiving data from client...");
    int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
    if (bytesReceived <= 0) {
        write_log_format(_WARN, "TCP Server - Failed to receive data or client disconnected. Bytes received: %d", bytesReceived);
    }
    return bytesReceived;
}

/**
 * Sends data to the client, converting to network byte order.
 *
 * @param clientSocket The client's socket.
 * @param response The data to be sent.
 * @param responseLength The length of the data in bytes.
 */
void send_to_client(SOCKET clientSocket, const char* response, int responseLength) {
    // Log the original response as a byte array
    write_log_byte_array(_DEBUG, response, responseLength);

    // Create a buffer to hold the converted data
    char* networkOrderBuffer = (char*)malloc(responseLength);
    if (networkOrderBuffer == NULL) {
        write_log_format(_ERROR, "TCP Server - Memory allocation failed.");
        return;
    }

    // Apply the byte order conversion
    for (int i = 0; i < responseLength; i += 8) { // assuming 8 bytes per integer (uint64_t)
        uint64_t* srcInt = (uint64_t*)(response + i);
        uint64_t* destInt = (uint64_t*)(networkOrderBuffer + i);

        // Convert to network byte order
        *destInt = htonll(*srcInt);  // Note: You may need to implement htonll if it's not available on your system
    }

    // Log the converted response as a byte array
    write_log_byte_array(_DEBUG, networkOrderBuffer, responseLength);

    int bytesSent = 0;
    int totalBytesSent = 0;

    while (totalBytesSent < responseLength) {
        bytesSent = send(clientSocket, networkOrderBuffer + totalBytesSent, responseLength - totalBytesSent, 0);
        if (bytesSent <= 0) {
            int error = WSAGetLastError();
            write_log_format(_ERROR, "TCP Server - Failed to send data. Bytes sent: %d, Error code: %d", bytesSent, error);
            free(networkOrderBuffer);  // Don't forget to free the buffer
            return;
        }
        totalBytesSent += bytesSent;
    }

    free(networkOrderBuffer);  // Don't forget to free the buffer
}

/**
 * Frees up the server and client socket resources and performs cleanup.
 *
 * @param serverSocket The server's socket.
 * @param clientSocket The client's socket.
 */
void cleanup_server(SOCKET serverSocket, SOCKET clientSocket) {
    write_log(_INFO, "TCP Server - Cleaning up server resources.");
    if (clientSocket) {
        write_log(_INFO, "TCP Server - Closing client socket.");
        closesocket(clientSocket);
    }
    if (serverSocket) {
        write_log(_INFO, "TCP Server - Closing server socket.");
        closesocket(serverSocket);
    }
    write_log(_INFO, "TCP Server - Cleaning up WinSock resources.");
    WSACleanup();
    write_log(_INFO, "TCP Server - Server cleanup complete.");
}
