#ifndef INITRD_H
#define INITRD_H

#include "types.h"

byteptr_t   initrd_get_ptr();
void        initrd_set_ptr(byteptr_t ptr);
void        initrd_list();
void        initrd_cat(const byteptr_t file);

#endif