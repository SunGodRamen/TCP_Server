// request_handler.h

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H
#pragma once

#include <stdint.h>
#include <time.h>
#include "logger.h"

typedef enum {
    OPERATION_GET_TIME,
    OPERATION_GET_RANDOM_NUMBER,
    OPERATION_GET_SERVER_NAME,
    UNKNOWN_OPERATION // Represents unrecognized sequences
} OperationType;

uint64_t handle_request(uint64_t* uri);

// Get the current timestamp in milliseconds since the Unix epoch.
#define URI_GET_TIME            0x1  // Binary: 1
uint64_t get_timestamp();

// Get a random 64-bit number.
#define URI_GET_RANDOM_NUMBER   0x2  // Binary: 10
uint64_t get_random_number();

// Get server name (represented as a 64-bit number)
#define URI_GET_SERVER_NAME     0x3  // Binary: 11
uint64_t get_server_name();

#endif
