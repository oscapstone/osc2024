
#include "mm/mm.h"
#include "io/dtb.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "peripherals/mailbox.h"

extern char __static_mem_start__, __static_mem_end__;

void buddy_init();

// the base of memory

memory_manager mem_manager;

U64* smalloc_ptr;

void *smalloc(U64 size) {

    U64 *result = smalloc_ptr;
    if ((U64)(smalloc_ptr + size) > &__static_mem_end__) {
        printf("Static memory can't allocate!\n");
        return 0;
    }
    
    smalloc_ptr += size;
    return result;
}

/**
 * Initialize this trash os memory management
*/
void mm_init() {
    // initialize the static malloc
    smalloc_ptr = (U64*)&__static_mem_start__;

    buddy_init();

    // get the memory size
    get_arm_mem();
    if (mailbox_call()) {
        mem_manager.base_ptr = mailbox[5];
        mem_manager.size = mailbox[6];
    } else {
        printf("Unable to query memory info!\n");
    }

}

void buddy_init() {

    // calculate frame table size
    U64 frames_size = mem_manager.size / MEM_FRAME_SIZE;
    mem_manager.frames = smalloc(frames_size);

}
