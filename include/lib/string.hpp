#pragma once

#include "util.hpp"

extern "C" {
void memzero(void* start, void* end);
void* memcpy(void* dst, const void* src, size_t n);
void memset(void* b, int c, size_t len);
}

int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dst, const char* src);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
const char* strchr(const char* s, char c);
const char* strchr_or_e(const char* s, char c);
long strtol(const char* s, const char** endptr = nullptr, size_t base = 0,
            size_t n = 0);
