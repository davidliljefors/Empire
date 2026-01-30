#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;


typedef struct emp_buffer
{
    u64 size;
    u8* data;
} emp_buffer;