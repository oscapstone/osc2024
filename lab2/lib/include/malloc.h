#ifndef __MEM__H__
#define __MEM_H__

#include "stdint.h"

/*
    | -------------------------------- |
    | prev_size/prev_data | chunk_size |
    |       8 bytes       |   8 bytes  |
    | -------------------------------- |
    |               data               |
    | -------------------------------- |
*/
typedef struct malloc_chunk {
    uint32_t prev_size;
    uint32_t chunk_size;
} malloc_chunk;

void* malloc(uint32_t);

#endif