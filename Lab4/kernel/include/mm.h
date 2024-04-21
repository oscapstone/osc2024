#ifndef MM_H
#define MM_H

#define PAGE_SHIFT    12
#define TABLE_SHIFT   9
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)  // 21

#define PAGE_SIZE    (1 << PAGE_SHIFT)     // 4K
#define SECTION_SIZE (1 << SECTION_SHIFT)  // 2M

#define LOW_MEMORY (2 * SECTION_SIZE)  // 0x400000

#ifndef __ASSEMBLER__

void memzero(unsigned long src, unsigned long n);

#endif

#endif /* MM_H */
