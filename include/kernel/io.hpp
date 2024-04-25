#pragma once
#include <cstdarg>

#include "string.hpp"
#include "util.hpp"

char kgetc();
char kgetc_sync();
void kputc(char c);
void kputc_sync(char c);

int PRINTF_FORMAT(1, 0) kvprintf(const char* format, va_list args);
int PRINTF_FORMAT(1, 2) kprintf(const char* format, ...);
int PRINTF_FORMAT(1, 0) kvprintf_sync(const char* format, va_list args);
int PRINTF_FORMAT(1, 2) kprintf_sync(const char* format, ...);

int PRINTF_FORMAT(1, 2) klog(const char* format, ...);

void kputs(const char* s);
void kputs_sync(const char* s);
void kprint_hex(string_view view);
void kprint_str(string_view view);
void kprint(string_view view);
