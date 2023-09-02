#include "tcp_server_thread.h"

#define BUFFER_SIZE MESSAGE_SIZE_BYTES

/**
 * TCP Server thread function.
 * Sets up and monitors the TCP server for incoming client messages,
 * and handles them appropriately.
 *
 * @param thread_config Configuration for this thread, including server parameters.
 * @return Always returns 0 upon termination.
 */
DWORD WINAPI tcp_server_thread(LPVOID thread_config) {
    write_log(_INFO, "TCP server thread started.");

    // Initialize TCP server
    uint64_t messageid = 0;
    SOCKET serverSocket = INVALID_SOCKET;
    server_thread_config* config = (server_thread_config*)thread_config;

    write_log(_INFO, "Initializing server socket.");
    serverSocket = init_server(config->server_config);

    SOCKET clientSocket;
    clientSocket = accept_connection(serverSocket);

    char clientMsg[BUFFER_SIZE];
    uint64_t message; // For simplicity, assuming all messages are 64 bits.
    while (1) {
        write_log(_DEBUG, "Waiting to read message from client.");
        uint64_t bytesRead = read_message_from_client(clientSocket, clientMsg);
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