#ifndef __FRAME_H__
#define __FRAME_H__

#include "type.h"

#define FRAME_SIZE_ORDER    12

void        frame_print_info();

void        frame_release(byteptr_t ptr);
byteptr_t   frame_alloc();
// byteptr_t   frame_request(uint32_t count);
// byteptr_t   frame_alloc_addr(uint32_t addr);

void        frame_system_init_frame_array(uint32_t start_addr, uint32_t end_addr);
uint32_t    frame_system_preserve(uint32_t start_addr, uint32_t end_addr);
void        frame_system_init_free_blocks();


#endif