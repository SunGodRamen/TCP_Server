#include "tcp_server.h"

int read_message_from_client(SOCKET clientSocket, char* buffer) {
    write_log(_DEBUG, "Starting read_message_from_client.");
    fd_set readSet;
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_usec = 0;

    FD_ZERO(&readSet);
    FD_SET(clientSocket, &readSet);
    int selectResult = select(clientSocket + 1, &readSet, NULL, NULL, &timeout);

    if (selectResult == SOCKET_ERROR) {
        write_log_format(_ERROR, "Select failed. Error Code: %d", WSAGetLastError());
        return -1;
    }
    if (selectResult == 0) {
        write_log(_WARN, "Client read timed out.");
        return 0; // Timeout
    }

    int totalBytesRead = 0;
    int bytesRead = 0;
    int bytesToRead = 8; // 64 bits

    while (totalBytesRead < bytesToRead) {
        bytesRead = recv(clientSocket, buffer + totalBytesRead, bytesToRead - totalBytesRead, 0);
        if (bytesRead == SOCKET_ERROR && WSAGetLastError() != 10053) {
            write_log_format(_ERROR, "Read from client failed. Error Code: %d", WSAGetLastError());
            return -1;
        }
        else if (bytesRead == 0) {
            write_log(_WARN, "Client disconnected before sending full message.");
            return totalBytesRead;
        }
        totalBytesRead += bytesRead;
    }

    write_log_byte_array(_DEBUG, buffer, totalBytesRead);
    write_log_format(_DEBUG, "Received %d bytes from client.", totalBytesRead);

    return totalBytesRead;
}

SOCKET init_server(uint16_t port) {
    write_log_format(_INFO, "Initializing server on port %d...", port);
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        write_log_format(_ERROR, "Failed to initialize WinSock. Error Code: %d", WSAGetLastError());
        exit(1);
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        write_log_format(_ERROR, "Failed to create socket. Error Code: %d", WSAGetLastError());
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        write_log_format(_ERROR, "Bind failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        write_log_format(_ERROR, "Listen failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    write_log(_INFO, "Server initialized successfully.");
    return serverSocket;
}

SOCKET accept_connection(SOCKET serverSocket) {
    write_log(_INFO, "Waiting for client connection...");
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        write_log_format(_ERROR, "Accept failed. Error Code: %d", WSAGetLastError());
        closesocket(serverSocket);
        exit(1);
    }

    write_log(_INFO, "Client connected.");
    return clientSocket;
}

int receive_from_client(SOCKET clientSocket, char* buffer, int bufferSize) {
    write_log(_DEBUG, "Receiving data from client...");
    int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
    if (bytesReceived <= 0) {
        write_log_format(_WARN, "Failed to receive data or client disconnected. Bytes received: %d", bytesReceived);
    }
    return bytesReceived;
}

void send_to_client(SOCKET clientSocket, const char* response, int responseLength) {
    // Log the original response as a byte array
    write_log_byte_array(_DEBUG, response, responseLength);

    // Create a buffer to hold the converted data
    char* networkOrderBuffer = (char*)malloc(responseLength);
    if (networkOrderBuffer == NULL) {
        write_log_format(_ERROR, "Memory allocation failed.");
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
            write_log_format(_ERROR, "Failed to send data. Bytes sent: %d, Error code: %d", bytesSent, error);
            free(networkOrderBuffer);  // Don't forget to free the buffer
            return;
        }
        totalBytesSent += bytesSent;
    }

    free(networkOrderBuffer);  // Don't forget to free the buffer
}

void cleanup_server(SOCKET serverSocket, SOCKET clientSocket) {
    write_log(_INFO, "Cleaning up server resources.");
    if (clientSocket) {
        write_log(_INFO, "Closing client socket.");
        closesocket(clientSocket);
    }
    if (serverSocket) {
        write_log(_INFO, "Closing server socket.");
        closesocket(serverSocket);
    }
    write_log(_INFO, "Cleaning up WinSock resources.");
    WSACleanup();
    write_log(_INFO, "Server cleanup complete.");
}