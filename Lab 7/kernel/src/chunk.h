#ifndef __CHUNK_H__
#define __CHUNK_H__

#include "type.h"

#define CHUNK_TABLE_ADDR    0x1000
#define CHUNK_TABLE_END     0x2000


void        init_chunk_table();

void        chunk_release(byteptr_t ptr);
byteptr_t   chunk_request(uint32_t size);

void        chunk_info(byteptr_t chunk);

uint32_t    chunk_max_size();
uint32_t    chunk_data_offset();

void        chunk_system_infro();

#endif