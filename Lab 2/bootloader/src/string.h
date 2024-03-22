#ifndef STRING_H
#define STRING_H

#include "type.h"

int32_t  str_cmp(const byteptr_t s1, const byteptr_t s2);
uint32_t str_eql(const byteptr_t s1, const byteptr_t s2);
uint32_t str_len(const byteptr_t str);

uint32_t ascii_dec_to_uint32(const byteptr_t str, uint32_t len);

#endif