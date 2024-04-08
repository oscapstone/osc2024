#ifndef _INITRD_H
#define _INITRD_H

#include "fdt.h"
#include "int.h"

extern char *initrd_base;

int initrd_addr(u32_t token, char *name, fdt_prop_t *prop, void *data);

#endif  // _INITRD_H
