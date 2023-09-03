// request_manager.c

#include "request_handler.h"

uint64_t handle_request(uint64_t uri) {
    // Handle the request based on the operation type
    switch (uri) {
    case OPERATION_GET_TIME:
        return get_timestamp();

    // Sample case for another operation *
//    case OPERATION_TWO:
//        return function_for_operation_2();

    case UNKNOWN_OPERATION:
    default:
        write_log_uint64_hex(_ERROR, "Request Handler - unknown request uri", uri);
        return 0; // or some error code in your protocol
    }
}

uint64_t get_timestamp() {
    // Assuming this function returns the current time in a format that fits in 64 bits.
    return (uint64_t)time(NULL) * 1000;
}

// Sample function implementation for another operation
//uint64_t function_for_operation_2() {
//    // Implement the logic for this operation
//    return 0; // Placeholder
//}
