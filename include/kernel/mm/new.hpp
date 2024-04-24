#pragma once

#include <cstdint>

using malloc_fp = void* (*)(uint64_t, uint64_t);
using free_fp = void (*)(void*);
void set_new_delete_handler(malloc_fp, free_fp);

void* operator new(unsigned long size);
void* operator new[](unsigned long size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
