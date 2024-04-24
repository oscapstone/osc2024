#pragma once

#include <cstdint>

void heap_info();
void heap_init();
void* heap_malloc(uint64_t size, uint64_t align = 1);
void heap_free(void* ptr);
