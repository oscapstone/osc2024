#ifndef _VM_H
#define _VM_H

#include "multitask.h"
#include "types.h"
#include "vm_macro.h"

void *map_pages(task_struct *thread, uint64_t user_addr, uint32_t size);

#endif