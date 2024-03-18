#pragma once

extern "C" {
void memzero(void* start, void* end);
void* memcpy(void* dst, const void* src, int n);
int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
}
