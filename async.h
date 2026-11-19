#pragma once
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
    #if __cplusplus >= 201103L
        #define ASYNC_NOEXCEPT noexcept
    #else
        #define ASYNC_NOEXCEPT throw()
    #endif
    extern "C" {
#else
    #define ASYNC_NOEXCEPT
#endif

#define ASYNC_STDIN_NAME            "stdin"
#define ASYNC_TYPE_INVALID          nullptr

typedef void* input_command_type;
#define input_command_is_valid(in)  ((in) != nullptr)

input_command_type connect(const char* input_file) ASYNC_NOEXCEPT;
long receive(input_command_type in, char* buf, size_t buf_size) ASYNC_NOEXCEPT;
void disconnect(input_command_type in) ASYNC_NOEXCEPT;


int file_is_good(input_command_type in) ASYNC_NOEXCEPT;

#ifdef __cplusplus
}
#endif
