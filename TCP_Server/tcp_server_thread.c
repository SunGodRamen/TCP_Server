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
    write_log(_INFO, "TCP Server Thread - TCP server thread started.");
    int ret = 0;  // Return code

    // Initialize TCP server
    uint64_t messageid = 0;
    SOCKET serverSocket = INVALID_SOCKET;
    server_thread_config* config = (server_thread_config*)thread_config;

    if (!config || !config->server_config) {
        write_log(_ERROR, "TCP Server Thread - Configuration or Server information is NULL.\n");
        ret = -1;  // Update return code to indicate error
        goto cleanup;
    }

    write_log(_INFO, "TCP Server Thread - Initializing server socket.");
    serverSocket = init_server(config->server_config);
    if (serverSocket == INVALID_SOCKET) {
        write_log(_ERROR, "TCP Server Thread - Failed to initialize server socket.");
        ret = -1;  // Update return code to indicate error
        goto cleanup;
    }

    SOCKET clientSocket;
    clientSocket = accept_connection(serverSocket);
    if (clientSocket == INVALID_SOCKET) {
        write_log(_ERROR, "TCP Server Thread - Failed to accept client connection.");
        ret = -1;  // Update return code to indicate error
        goto cleanup;
    }

    char clientMsg[BUFFER_SIZE];
    uint64_t message; // For simplicity, assuming all messages are 64 bits.
    while (1) {
        write_log(_DEBUG, "TCP Server Thread - Waiting to read message from client.");
        uint64_t bytesRead = read_message_from_client(clientSocket, clientMsg);
        if (bytesRead == sizeof(uint64_t)) {  // Ensure we read a full 64-bit message.
            write_log(_INFO, "TCP Server Thread - Full 64-bit message received from client.");
            // Convert the received message into a 64-bit number
            memcpy(&message, clientMsg, sizeof(uint64_t));
            // Send a confirmation for the received message
            messageid++;
            uint64_t confirmation;
            encode_confirmation(messageid, &confirmation);  // Assuming that the 'message' contains the request ID
            send_to_client(clientSocket, (const char*)&confirmation, sizeof(confirmation));
            write_log_format(_INFO, "TCP Server Thread - Sent confirmation %llx to client.", confirmation);

            // Interpret and handle the message
            MessageType messageType = { 0 };
            interpret_message(&message, &messageType);
            switch (messageType) {
            case REQUEST_MESSAGE: {
                uint64_t uri;
                extract_request_uri(&message, &uri);
                uint64_t response_data = handle_request(&uri);
                uint64_t response = 0;
                encode_response(message, response_data, &response);

                // Now send the response back to the client.
                send_to_client(clientSocket, (const char*)&response, sizeof(uint64_t));
                write_log_format(_INFO, "TCP Server Thread - Sent response %llx to client.", response);

                break;
            }
            case CONFIRM_MESSAGE:
                write_log(_ERROR, "TCP Server Thread - Unexpected confirm message type received.");
                break;
            default:
                write_log(_ERROR, "TCP Server Thread - Unrecognized or unhandled message type received.");
                break;
            }
        }
        else {
            write_log(_WARN, "TCP Server Thread - Incomplete message received from client.");
        }
    }

cleanup:
    write_log(_INFO, "TCP Server Thread - Starting cleanup process.");

    // Close the server socket if it's valid
    if (serverSocket != INVALID_SOCKET) {
        cleanup_server(serverSocket, clientSocket);
    }

    // Free the configuration structure
    if (config) {
        free(config);
    }

    write_log(_INFO, "TCP Server Thread - TCP server thread terminated.");
    return ret;  // Return the final result code
}
