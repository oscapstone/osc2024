#pragma once

using malloc_fp = void* (*)(unsigned long, unsigned long);
using free_fp = void (*)(void*);
void set_new_delete_handler(malloc_fp, free_fp);

void* operator new(unsigned long size);
void* operator new[](unsigned long size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
