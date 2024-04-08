#ifndef _INITRD_H
#define _INITRD_H

#include "fdt.h"
#include "int.h"

extern char *initrd_base;

int initrd_addr(fdt_node_t node);

#endif  // _INITRD_H
