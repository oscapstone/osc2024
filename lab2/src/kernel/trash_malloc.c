#include "trash_malloc.h"
#include "utils.h"

unsigned long* addr;

#define TRASH_MAX_SIZE 4096
static unsigned char allocate_buffer[TRASH_MAX_SIZE];
static unsigned long allocate_offset = 0;

void* trash_malloc(unsigned long size) {

    // align with 64 bits
    align(&size, 8);

    if (allocate_offset + size > TRASH_MAX_SIZE) {
        return (void*)0;
    }
    void* allocation = (void*)&allocate_buffer[allocate_offset];
    allocate_offset += size;

    return allocation;
}