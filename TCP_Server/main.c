#include "tcp_server.h"
#include "logger.h"
#include "message_protocol.h"
#include "request_handler.h"

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NUM_PORTS 1
const int TCP_PORTS[NUM_PORTS] = { 4000 };

#define MAX_MSG_SIZE 1024

char* LOG_FILE = "C:\\Users\\avons\\Code\\Anatomic\\TCP_Server\\logs\\TCP_Server.log";
#define LOG_LEVEL _WARN

// Setup and Monitor the TCP server for messages from the Client
DWORD WINAPI monitor_tcp(LPVOID port) {
    uint64_t messageid = 0;
    // Initialize TCP server
    write_log(_INFO, "Initializing TCP server...");
    uint16_t portNumber = (uint16_t)(*((int*)port));
    SOCKET serverSocket = init_server(portNumber);

    SOCKET clientSocket;
    clientSocket = accept_connection(serverSocket);

    char clientMsg[MAX_MSG_SIZE];
    uint64_t message; // For simplicity, assuming all messages are 64 bits.
    while (1) {
        write_log(_DEBUG, "Waiting to read message from client.");
        uint64_t bytesRead = read_message_from_client(clientSocket, clientMsg);
        //write_log_uint64_hex(_DEBUG, "received",&clientMsg);
        if (bytesRead == sizeof(uint64_t)) {  // Ensure we read a full 64-bit message.
            write_log(_INFO, "Full 64-bit message received from client.");
            // Convert the received message into a 64-bit number
            memcpy(&message, clientMsg, sizeof(uint64_t));
            // Send a confirmation for the received message
            messageid++;
            uint64_t confirmation;
            encode_confirmation(messageid, &confirmation);  // Assuming that the 'message' contains the request ID
            send_to_client(clientSocket, (const char*)&confirmation, sizeof(confirmation));
            write_log_format(_INFO, "Sent confirmation %llx to client.", confirmation);

            // Interpret and handle the message
            MessageType messageType = { 0 };
            interpret_message(message, &messageType);
            switch (messageType) {
            case REQUEST_MESSAGE: {
                uint64_t uri;
                extract_request_uri(message, &uri);
                uint64_t response_data = handle_request(uri);
                uint64_t response = 0;
                if (response_data != 0) {
                    encode_response(message, response_data, &response);
                }

                // Now send the response back to the client.
                send_to_client(clientSocket, (const char*)&response, sizeof(uint64_t));
                write_log_format(_INFO, "Sent response %llx to client.", response);
                write_log_byte_array(_DEBUG, response, sizeof(uint64_t));

                break;
            }
            case CONFIRM_MESSAGE:
                // You may not need to do anything here as the confirmation was already sent above.
                break;
            default:
                write_log(_ERROR, "TCP: Unrecognized or unhandled message type received.");
                break;
            }
        }
        else {
            write_log(_WARN, "Incomplete message received from client.");
        }
    }

    write_log(_INFO, "Exiting monitor_tcp thread.");
    cleanup_server(serverSocket, clientSocket);
    return 0;
}

int main() {
    write_log(_INFO, "Main function started.");

    init_logger(LOG_FILE);
    set_log_level(LOG_LEVEL);

    HANDLE tcpThreads[NUM_PORTS];

    for (int i = 0; i < NUM_PORTS; i++) {
        tcpThreads[i] = CreateThread(NULL, 0, monitor_tcp, &TCP_PORTS[i], 0, NULL);
        if (tcpThreads[i] == NULL) {
            // handle thread creation error
            write_log(_ERROR, "Error creating thread for a port.");
        }
    }

    // Wait for all threads to exit
    for (int i = 0; i < NUM_PORTS; i++) {
        if (tcpThreads[i]) {
            WaitForSingleObject(tcpThreads[i], INFINITE);
            CloseHandle(tcpThreads[i]);
        }
    }

    close_logger();
    write_log(_INFO, "Main function exiting.");

    return 0;
}