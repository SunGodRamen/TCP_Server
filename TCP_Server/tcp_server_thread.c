#include "tcp_server_thread.h"

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

    char clientMsg[MESSAGE_SIZE_BYTES];
    char confirmMsg[MESSAGE_SIZE_BYTES];
    char responseMsg[MESSAGE_SIZE_BYTES];
    while (1) {
        write_log(_DEBUG, "TCP Server Thread - Waiting to read message from client.");
        int bytesRead = read_message_from_client(clientSocket, clientMsg, MESSAGE_SIZE_BYTES);
        if (bytesRead == MESSAGE_SIZE_BYTES) {  // Ensure we read a full 64-bit message.
            write_log_format(_INFO, "TCP Server Thread - Full %d-byte message received from client.", MESSAGE_SIZE_BYTES);
            // Send a confirmation for the received message
            messageid++;
            encode_confirmation(&confirmMsg, messageid, 0x01);  // Assuming that the 'message' contains the request ID
            send_to_client(clientSocket, (const char*)&confirmMsg, sizeof(confirmMsg));
            write_log(_INFO, "TCP Server Thread - Sent confirmation to client.");
            write_log_byte_array(_INFO, confirmMsg, sizeof(confirmMsg));

            // Interpret and handle the message
            MessageType messageType = { 0 };
            interpret_message(&clientMsg, &messageType);
            switch (messageType) {
            case REQUEST_MESSAGE: {
                uint64_t uri;
                extract_request_uri(&clientMsg, &uri);
                uint64_t response_data = handle_request(&uri);
                encode_response(responseMsg, messageid, &response_data);

                // Now send the response back to the client.
                send_to_client(clientSocket, (const char*)&responseMsg, sizeof(responseMsg));
                write_log(_INFO, "TCP Server Thread - Sent response to client.");

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
