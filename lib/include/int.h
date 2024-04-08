#ifndef _INT_H
#define _INT_H

typedef unsigned int u32_t;
typedef unsigned long long size_t;
typedef unsigned long long u64_t;

int align(int num, int n);

/*
 * Convert big endian number to little endian one. (32bit)
 */
u32_t be2le_32(u32_t be);

#endif  // _INT_H
