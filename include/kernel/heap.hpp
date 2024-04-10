#pragma once

extern char __heap_start[];
extern char __heap_end[];
extern char* heap_cur;

void heap_reset();
void* heap_malloc(int size, int align = 1);
bool heap_free(int size);

void* operator new(unsigned long size);
void* operator new[](unsigned long size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
