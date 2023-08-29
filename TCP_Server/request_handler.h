// request_handler.h

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H
#pragma once

#include <stdint.h>
#include <time.h>

#include "logger.h"

typedef enum {
    OPERATION_GET_TIME,
//  OPERATION_TWO,
    UNKNOWN_OPERATION // Represents unrecognized sequences
} OperationType;

uint64_t handle_request(uint64_t uri);

// Get the current timestamp in milliseconds since the Unix epoch.
#define URI_GET_TIME      0x4000000000000000
uint64_t get_timestamp();

// Sample function for implementation for another recognized action
//uint64_t function_for_operation_2();

#endif