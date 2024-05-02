# Lab4
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab4.html)
---
## Basic Exercises
** Remember that ```(struct*)cur++``` is to add the ```sizeof(struct)```, so if you wanna directly manipulate the address you must cast it to something like ```void*```**
** Don't do something like ```cur += amount * sizeof(struct)```, which will lead to a square of memory space consumption ** 
### Basic Exercise 1 - Buddy System
+ background
+ In this part, all metadata and available memory are stired in 0x10000000~0x20000000. As for the ```val``` of each block metadata, -3 indicates that it's divided into smaller blocks, -2 indicate that it belongs to a larger contiguous memory block, and -1 indicates that is already allocated
+ Allocation
    + The idea will be first put whole metadata into ```BUDDY_METADATA_ADDR```, which starting with one ```buddy_system_t``` struct then ```(1 << MAX_ORDER) - 1``` ```buddy_block_list_t``` structs. The rest of the space will be used for memory allocation.
    + Defina a struct ```buddy_block_list_t``` as the metadata of every memory block
    + Define a struct ```buddy_system_t``` as the buddy system metadata and it contains
        1. ```buddy_block_list_t **buddy_list```: a 2D linked lists, which is the metadata of every memory block
        2. ```int first_avail[MAX_ORDER]```     : Store the first available block index of every list(the index amount is constant, so for 3rd list, its index of blocks will be 0->4->8, store 4 as first_avail if 0 is allocated)
        3. ```void* list_addr[MAX_ORDER];```    : Store the address of every list's metadata(i.e. the address of every ```*buddy_list```)
    + ```buddy_init``` should be used when the buddy system is first called(malloc) as it will initialize all metadata and allocate memory space to each block
    + ```buddy_malloc``` will
        1. Iterate every order level to find a order that is at least larger or equal to the requested size, and there's a free block in that order level
        2. Use ```buddy_split``` to further split current block into smaller block but still larger than requested size by utilizing the start index and end index 
            - The concept is:
                - Always use the leftest block -> Mark the first block within the range as allocated(-3 is not level 0), others marked as usable.
                - Divide the end index into half if next level can still satisify requested size
        3. Use ```mark_allocated```mark the lower blocks(which is the block we get from previous step) within same range as allocated
        4. update ```first_avail```
+ Free
    1. First, we free all its child at lower layer by marking them with -2
    2. Use address to get block index
    3. use index to get the block metadata(iterate until larger block is not -1, meaning that we reach the target block as larger block will also make smaller ones become -1)
    4. Check if buddy(obtained by ```block_index ^ (1 << block_order)```) is also available, if it is, merge(by marking it -2) and iterate to higher level of block metadata.
### Basic Exercise 2 - Dynamic Memory Allocator 
+ Background
- Allocation
    - Create pools of size ```{16, 32, 48, 196, 512, 1024}```, which can be modified in the future.
    - ```int pool_page_addr[(1 << (MAX_ORDER - 1))]``` will be used to recorded which pools that page frames belongs to
    - ```int free_list_counts[NUM_POOLS]``` will be used to record the number of free small blocks in each pool
    1. First check the requested size, if the size exceeded the largest pool size, then use ```buddy_malloc()``` to return the multiple of page frame
    2. If the pool is empty, allocate a page and split that page into many small blocks with same pool size and updat the free pool list.
    3. Use the free pool list to return a small block with closest size.

## Advanced Exercises
### Advanced Exercise 1 - Efficient Page Allocation
+ Background
+ I should already implemented basic exercise in O(logn) as I access the metadata in order level instead of iterate all indexs.


### Advanced Exercise 2 - Reserved Memory 
+ First calculate the index for both start and end address, and if the end index is smaller than start index, meaning it's an invalid request
+ Since the block index of all levels are the same, so I use the start index and end index to go through all levels, and mark lowest level(0) blocks as allocated(-1), higher level blocks as dividing into smaller blocks(-3)
+ if the buddy is not allocated, mark it as usable as higher level will be marked as -3

### Advanced Exercise 3 - Startup Allocation
+ Reserve Spin tables for multicore boot 
+ Reserve Kernel image in the physical memory ```memory_reserve((void*)&_start, (void*)&__end);```
+ Reserve the CPIO archive in the physical memory