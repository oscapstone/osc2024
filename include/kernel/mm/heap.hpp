#pragma once

#include "util.hpp"

constexpr uint32_t chunk_size[] = {
    0x10, 0x30, 0x50, 0xf0, 0x110, 0x330, 0x550,
};

constexpr int chunk_class = sizeof(chunk_size) / sizeof(uint32_t);
constexpr uint32_t max_chunk_size = chunk_size[chunk_class - 1];

void heap_info();
void heap_init();
void* heap_malloc(uint64_t size);
void heap_free(void* ptr);
