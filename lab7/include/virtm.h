#pragma once

#include <stdint.h>

#include "hardware.h"
#include "scheduler.h"

#define USER_STACK 0x0000FFFFFFFFB000

#define PTE_NORMAL_ATTR \
  (PD_ACCESS | PD_UKACCESS | PD_MAIR_NORMAL_NOCACHE | PD_ENTRY)

void map_pages(uintptr_t pgd, uintptr_t va, uintptr_t size, uintptr_t pa);

void mapping_user_thread(thread_struct *thread, int gpu_mem_size);