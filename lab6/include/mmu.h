#ifndef _MMU_H
#define _MMU_H


#define PAGE_TABLE_SIZE 0x1000

#define PT_R    0x0001
#define PT_W    0x0002
#define PT_X    0x0004

#include <stdint.h>

/*
 * Set identity paging, enable MMU
 */
void mmu_init(void);

uint64_t *pt_create(void);
void pt_free(uint64_t *pt);

void pt_map(uint64_t *pt, void *va, uint64_t size, void *pa, uint64_t flag);


#endif /* _MMU_H */