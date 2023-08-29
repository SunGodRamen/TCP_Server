#include "tcp_server.h"

/**
 * Reads a 64-bit (8 bytes) message from the client.
 *
 * @param clientSocket The client socket to read the message from.
 * @param buffer The buffer to store the message.
 * @return The number of bytes read, or -1 on error.
 */
int read_message_from_client(SOCKET clientSocket, char* buffer) {
    int totalBytesRead = 0;
    int bytesRead = 0;
    int bytesToRead = 8; // 64 bits

    while (totalBytesRead < bytesToRead) {
        bytesRead = recv(clientSocket, buffer + totalBytesRead, bytesToRead - totalBytesRead, 0);
        if (bytesRead == SOCKET_ERROR && WSAGetLastError() != 10053) {
            return -1;
        }
        else if (bytesRead == 0) {
            // Client disconnected before sending full message.
            return totalBytesRead;
        }
        totalBytesRead += bytesRead;
    }

    return totalBytesRead;
}

SOCKET init_server(uint16_t port) {
    WSADATA wsaData;

    // Initialize WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {

        char buffer[MAX_LOG_SIZE];
        snprintf(buffer, sizeof(buffer), "TCP: Failed to initialize WinSock. Error Code: %d", WSAGetLastError());
        write_log(buffer);
        memset(buffer, 0, sizeof(buffer));

        exit(1);  // Exit the program if WinSock initialization fails
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {

        char buffer[MAX_LOG_SIZE];
        snprintf(buffer, sizeof(buffer), "TCP: Failed to create socket. Error Code: %d\n", WSAGetLastError());
        write_log(buffer);
        memset(buffer, 0, sizeof(buffer));

        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {

        char msg_buffer[MAX_LOG_SIZE];
        snprintf(msg_buffer, sizeof(msg_buffer), "TCP: Bind failed. Error Code: %d\n", WSAGetLastError());
        write_log(msg_buffer);
        memset(msg_buffer, 0, sizeof(msg_buffer));

        closesocket(serverSocket);
        exit(1);
    }

    // Listen for connections
    if (listen(serverSocket, 3) == SOCKET_ERROR) {

        char msg_buffer[MAX_LOG_SIZE];
        snprintf(msg_buffer, sizeof(msg_buffer), "TCP: Listen failed. Error Code: %d\n", WSAGetLastError());
        write_log(msg_buffer);
        memset(msg_buffer, 0, sizeof(msg_buffer));

        closesocket(serverSocket);
        exit(1);
    }

    return serverSocket;
}

SOCKET accept_connection(SOCKET serverSocket) {
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {

        char msg_buffer[MAX_LOG_SIZE];
        snprintf(msg_buffer, sizeof(msg_buffer), "TCP: Accept failed. Error Code: %d\n", WSAGetLastError());
        write_log(msg_buffer);
        memset(msg_buffer, 0, sizeof(msg_buffer));

        closesocket(serverSocket);
        exit(1);
    }

    return clientSocket;
}

int receive_from_client(SOCKET clientSocket, char* buffer, int bufferSize) {
    return recv(clientSocket, buffer, bufferSize, 0);
}

void send_to_client(SOCKET clientSocket, const char* response, int responseLength) {
    send(clientSocket, response, responseLength, 0);
}

void cleanup_server(SOCKET serverSocket, SOCKET clientSocket) {
    if (clientSocket) {
        closesocket(clientSocket);
    }
    if (serverSocket) {
        closesocket(serverSocket);
    }
    WSACleanup();
}
