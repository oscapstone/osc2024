#ifndef	_MM_H
#define	_MM_H

#define    PAGE_SHIFT   12
#define   TABLE_SHIFT   9
#define SECTION_SHIFT	(PAGE_SHIFT + TABLE_SHIFT)
#define    PAGE_SIZE   	(1 << PAGE_SHIFT)	
#define SECTION_SIZE   	(1 << SECTION_SHIFT)	


#define KERNEL_START_ADDR   (char*)0x80000
#define HEAP_MAX            (char*)0x100000
#define USER_PROCESS_ADDR   (char*)0x120000
#define USER_PROCESS_SP     (char*)0x140000
#define LOW_MEMORY          (2 * SECTION_SIZE)


#ifndef __ASSEMBLER__
void memzero(unsigned long src, unsigned long n);
void* simple_malloc(unsigned int bytes);
#endif

#endif  /*_MM_H */

