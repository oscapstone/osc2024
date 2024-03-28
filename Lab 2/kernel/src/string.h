#ifndef __STRING_H__
#define __STRING_H__

#include "type.h"

int32_t     str_cmp(const byteptr_t s1, const byteptr_t s2);
uint32_t    str_eql(const byteptr_t s1, const byteptr_t s2);
uint32_t    str_len(const byteptr_t str);
byteptr_t   str_tok(byteptr_t s);

void        uint32_to_hex(uint32_t n, byteptr_t buffer);
void        uint32_to_ascii(uint32_t n, byteptr_t buffer);

uint32_t    ascii_to_uint32(const byteptr_t s, uint32_t len);
uint64_t    ascii_to_uint64(const byteptr_t s, uint32_t len);

uint32_t    ascii_dec_to_uint32(const byteptr_t str);

#endif