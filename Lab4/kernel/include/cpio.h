#ifndef CPIO_H
#define CPIO_H

#include "int.h"

#define CPIO_MAGIC        "070701"
#define CPIO_MAGIC_FOOTER "TRAILER!!!"

#define CPIO_EXIT_SUCCESS 0
#define CPIO_EXIT_ERROR   1


void ls(void);
void cat(char*);
void exec(char*);

uintptr_t get_cpio_start(void);
void set_cpio_start(uintptr_t);

uintptr_t get_cpio_end(void);
void set_cpio_end(uintptr_t);

int cpio_init(void);

#endif /* CPIO_H */
