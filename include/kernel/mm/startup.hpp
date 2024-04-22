#pragma once

void startup_alloc_info();
void startup_alloc_init();
void* startup_malloc(unsigned long size, unsigned long align = 1);
void startup_free(void* ptr);
bool startup_free(unsigned long size);
