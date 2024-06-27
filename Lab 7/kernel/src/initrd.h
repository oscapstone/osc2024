#ifndef __INITRD_H__
#define __INITRD_H__

#include "type.h"


byteptr_t   initrd_get_ptr();
void        initrd_set_ptr(byteptr_t ptr);
byteptr_t   initrd_get_end();
void        initrd_set_end(byteptr_t ptr);

void        initrd_list();
void        initrd_cat(const byteptr_t file);
void        initrd_exec(const byteptr_t name);
void        initrd_exec_replace(const byteptr_t name);

// int         initrd_call_exec(const char* name, char *const argv[]);


typedef void (cpio_cb) (byteptr_t name,  byteptr_t content, uint32_t size);

void initrd_traverse(cpio_cb * cb);

#endif