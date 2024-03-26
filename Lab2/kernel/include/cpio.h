#ifndef CPIO_H
#define CPIO_H

#include "int.h"

#define CPIO_MAGIC        "070701"
#define CPIO_MAGIC_FOOTER "TRAILER!!!"

#define CPIO_EXIT_SUCCESS 0
#define CPIO_EXIT_ERROR   1

void ls(void);
void cat(char*);

uintptr_t get_cpio_ptr(void);
void set_cpio_ptr(uintptr_t);

#endif /* CPIO_H */
