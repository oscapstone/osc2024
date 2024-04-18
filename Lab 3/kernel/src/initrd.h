#ifndef __INITRD_H__
#define __INITRD_H__

#include "type.h"

byteptr_t   initrd_get_ptr  ();
void        initrd_set_ptr  (byteptr_t ptr);
void        initrd_list     ();
void        initrd_cat      (const byteptr_t file);
void        initrd_exec     (const byteptr_t name);

#endif