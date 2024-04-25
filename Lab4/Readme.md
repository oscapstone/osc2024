# OSC2024 Lab4
Author: jerryyyyy708 (just to make sure my code is not copied by anyone)
## Buddy System
Page frame allocation algorithm to seperate address into 2^i * 4096.
* physical address: index * 4096 + base
* size: 4096 * 2 ^ order

### Page Frame Allocator
For virtual memory mapping (running user program in user space), need a lot of 4KB memory blocks since it is the unit of mapping. 

Regular dynamic memory allocator uses size header before memory block -> additional space. (can't be use as page frame, not aligned)

Page frame allocator can allocate contiguous page frame for large buffers.

If only run in kernel space, the process and the memory usage can be predicted(usually) thus in this case dynamic memory allocator might be enough.

### Why 4KB
* 4KB is a page frame size for many operating system for mapping virtual memory and alignment. (MMU: this size often coincides with the page table entry size used by the processor's Memory Management Unit (MMU). This alignment helps optimize the efficiency of hardware memory access.)
* Tradeoff
    * smaller frame size: less internal fragment problem, but has a larger page table which requires more memory and tranverse time for virtual memory.
    * larger frame size: more internal fragment problem but smaller page table.

### Find buddy
use index xor 2^order to find buddy.
```
int bud = i ^ (1 << frames[i].order); //shift order bits, every shift means * 2
```
xor: no change if meet 0, change if meet 1 -> the xor simply change the bit of the order i, which means +-2^order.

### Freelist
Keep a list of free pages in each order for efficient allocation in O(logn).

## Dynamic Memory Allocator
Translate page to smaller memory for small memory allocation.

## Memory Reserve 
Make sure some in use memory are not allocated.
* Spin table 0x0000~0x1000: A table for multicore to spin and wait for start. The spin table from 0x0000 to 0x1000 serves as a crucial component for the synchronized startup of multicore processors. Each processor core consults this table to decide when to initiate operations, ensuring all cores start in a coordinated manner
* Kernel image: 0x80000 to _end (and stack, buddy system)
* Startup allocator (also in kernel, bss)
* iniramfs: get info by dtb
* Devicetree: start address in x0 and end address by header

## Startup Allocator
The physical address is determined in runtime, so we cannot know how large frame array we need. Therefore, we need a startup allocator to allocate appropriate memory for the frame array instead of hardcode them in bss.

## Demo Note
1. Get memory location
2. Show status instruction and start startup allocation
3. Show frame count
4. Show memory reserve
5. Startup merging and show freelist
6. Allocate all page with order lower than 6
7. Show freelist
8. Show releasing redundant memory block
9. Show freelist
10. Free the order 0 page to show merging iteratively
11. Print freelist 
12. Show dynamic allocator and free by allocating 500 16byte chunck, show content and free, then malloc again

## Implementation Index
```
//structures
typedef struct frame
typedef struct freelist
typedef struct memory_pool

//startup allocator
void *simple_alloc

//freelist
void remove_from_freelist
void insert_into_freelist

//demo utils
void status_instruction()
void print_freelist(int head)
void convert_val_and_print(int start, int len)
void allocate_all()
void demo_page_alloc()

//reserve memory
void memory_reserve(unsigned long start, unsigned long end)

//page frame allocator
void frames_init()
void merge_free(frame_t *frame, int print)
void merge_all(int print)
void free_page(unsigned long address)
int get_order(unsigned long size)
void* allocate_page(unsigned long size)

//dynamic memory allocator
void init_memory()
void * malloc(unsigned long size)
void free(void* ptr) 
```

## TODO
1. Add hardcode version to the vm 