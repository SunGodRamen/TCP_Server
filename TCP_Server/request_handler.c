// request_handler.c

#include "request_handler.h"
#include <stdlib.h>

uint64_t handle_request(uint64_t* uri) {
    // Handle the request based on the operation type
    write_log_uint64_hex(_INFO, "Request Handler - uri", uri);
    write_log_uint64_hex(_INFO, "Request Handler - URI_GET_TIME", URI_GET_TIME);
    write_log_uint64_hex(_INFO, "Request Handler - URI_GET_RANDOM_NUMBER", URI_GET_RANDOM_NUMBER);
    write_log_uint64_hex(_INFO, "Request Handler - URI_GET_SERVER_NAME", URI_GET_SERVER_NAME);
    switch (*uri) {
    case URI_GET_TIME:
        return get_timestamp();

    case URI_GET_RANDOM_NUMBER:
        return get_random_number();

    case URI_GET_SERVER_NAME:
        return get_server_name();

    default:
        write_log_format(_ERROR, "Request Handler - unknown request uri: %llu", uri);
        return 0; // or some error code in your protocol
    }
}

uint64_t get_timestamp() {
    // Assuming this function returns the current time in a format that fits in 64 bits.
    return (uint64_t)time(NULL) * 1000;
}

uint64_t get_random_number() {
    // Generates a pseudo-random 64-bit integer
    return (uint64_t)rand() << 32 | rand();
}

uint64_t get_server_name() {
    // For demonstration, the server name is represented as a 64-bit number.
    // In a real-world application, you would probably send a string.
    return 0x537276724e6d6500; // "SrverNme" in ASCII as a 64-bit integer
}
