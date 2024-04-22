#pragma once

void startup_alloc_info();
void startup_alloc_reset();
void* startup_malloc(int size, int align = 1);
bool startup_free(int size);
