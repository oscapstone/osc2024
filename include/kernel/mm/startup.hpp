#pragma once

#include <cstdint>

extern char __heap_start[];
extern char __heap_end[];

void startup_alloc_info();
void startup_alloc_init();
void* startup_malloc(uint64_t size, uint64_t align = 1);
void startup_free(void* ptr);
bool startup_free(uint64_t size);
