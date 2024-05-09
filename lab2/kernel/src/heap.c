#include "heap.h" // Include the header file for heap management

extern char _heap_top;              // External variable indicating the start of the heap
static char *htop_ptr = &_heap_top; // Static pointer to keep track of the current top of the heap

// Function to allocate memory from the heap
void *malloc(unsigned int size)
{
    // Explanation of heap layout (not actual code):
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // Allocate space for the heap_block header, which is 0x10 bytes
    char *r = htop_ptr + 0x10;
    // Round up the requested size to a multiple of 0x10 for alignment
    size = 0x10 + size - size % 0x10;
    // Store the size of the allocated block (including the header) at the beginning of the block
    *(unsigned int*)(r - 0x8) = size;

    // total size of header (0x10) + allocated size (e.g.0x18 -> 0x20) = 0x30
    unsigned int total_size = 0x10 + size;
        
    // Move the heap top pointer forward by the size of the allocated block
    htop_ptr += total_size;
    // Return a pointer to the allocated memory (excluding the header)
    return r;
}