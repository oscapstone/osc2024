
#include <math.h>

#include "mm/mm.h"
#include "io/dtb.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "peripherals/mailbox.h"
#include "io/dtb.h"

extern void* _dtb_ptr;
extern char __static_mem_start__, __static_mem_end__;
extern char __kernel_start__, __kernel_end__;

void buddy_init();
void mem_pool_init();
U32 mem_buddy_alloc(U8 order);
int mem_put_free(U32 frame_index, U8 order);
void mem_memory_reserve(UPTR start, UPTR end);

// the base of memory

MEMORY_MANAGER mem_manager;

#define MEM_ADDR_TO_INDEX(x)    ((((UPTR)x & -MEM_FRAME_SIZE) - mem_manager.base_ptr) / MEM_FRAME_SIZE)
#define MEM_INDEX_TO_ADDR(x)    ((x * MEM_FRAME_SIZE) + (char*)mem_manager.base_ptr)

U64* smalloc_ptr;
U64* smalloc_end_ptr;

void *smalloc(U64 size) {

    U64 *result = smalloc_ptr;
    if ((UPTR)(smalloc_ptr + size) > (UPTR)&__static_mem_end__) {
        printf("Static memory can't allocate!\n");
        return 0;
    }
    
    smalloc_ptr += size;
    return result;
}

UPTR initramfs_start, initramfs_end;

void get_reserve_memory_addr(int token, const char* name, const void* data, U32 size) {
    // initramfs
    if (token == FDT_PROP && (utils_strncmp(name, "linux,initrd-start", 18) == 0)) {
        initramfs_start = utils_transferEndian(*((U32*)data));
    } else if (token == FDT_PROP && (utils_strncmp(name, "linux,initrd-end", 18) == 0)) {
        initramfs_end = utils_transferEndian(*((U32*)data));
    }
}

/**
 * Initialize this trash os memory management
*/
void mm_init() {
    // initialize the static malloc
    smalloc_ptr = (U64*)&__static_mem_start__;
    smalloc_end_ptr = (U64*)&__static_mem_end__;

    // use the example base offset (or use the kernel end ptr)
    //mem_manager.base_ptr = 0x10000000;

    // get the memory size
    get_arm_mem();
    if (mailbox_call()) {
        mem_manager.base_ptr = mailbox[5];
        // end - start
        mem_manager.size = mailbox[6] - mem_manager.base_ptr;
    } else {
        printf("Unable to query memory info!\n");
    }

    mem_manager.number_of_frames = mem_manager.size / MEM_FRAME_SIZE;
    NS_DPRINT("[MEMORY][TRACE] Total frames: %d\n", mem_manager.number_of_frames);
    mem_manager.frames = smalloc(mem_manager.number_of_frames * sizeof(MEM_FRAME_INFO_SIZE));

    mem_manager.levels = utils_highestOneBit(mem_manager.number_of_frames);

    mem_manager.free_list = smalloc(mem_manager.levels * sizeof(FREE_INFO));

    // put the memory reserve code here
    mem_memory_reserve(          0, 0x1000);       // spin table for multiboot
    mem_memory_reserve(     0x1000, 0x3000);       // identity MMU map
    mem_memory_reserve((UPTR)&__kernel_start__, (UPTR)&__kernel_end__);     // kernel code

    // device tree
    struct fdt_header* header = (struct fdt_header*)_dtb_ptr;
    U32 magic = utils_transferEndian(header->magic);
    U32 totalSize = utils_transferEndian(header->totalsize);

    if (magic != 0xd00dfeed) {
        printf("[MEMORY][ERROR] Device tree magic not correct. magic: %x\n", magic);
    }
    mem_memory_reserve((UPTR)_dtb_ptr, (UPTR)_dtb_ptr + totalSize);
    NS_DPRINT("[MEMORY][TRACE] Device tree reserve. offset: %x, size: %d\n", (U64)_dtb_ptr, totalSize);

    fdt_traverse(get_reserve_memory_addr);

    mem_memory_reserve(initramfs_start, initramfs_end);
    NS_DPRINT("[MEMORY][TRACE] initramfs reserve. offset: %x, size: %x\n", initramfs_start, initramfs_end);


    buddy_init();

    // TODO: reserve the memory in buddy system

    // init dynamic memory pools
    mem_pool_init();
}

void mem_pool_init() {
    mem_manager.pools = smalloc(MEM_MEMORY_POOL_ORDER * sizeof(MEMORY_POOL));

    for (int i = 0; i < MEM_MEMORY_POOL_ORDER; i++) {
        MEMORY_POOL* pool = &mem_manager.pools[i];
        pool->obj_size = MEM_MIN_OBJECT_SIZE * (i + 1);
        pool->slot_size = MEM_FRAME_SIZE / pool->obj_size;
        pool->chunk_count = 0;      // no chunk has been allocated
        pool->chunks = smalloc(MEM_MAX_CHUNK * sizeof(CHUNK_INFO));
        memzero(pool->chunks->slots, sizeof(pool->chunks->slots));
    }
}

/**
 * A clumsy approach to Linux slab
*/
void* mem_chunk_alloc(U32 size) {
    if (size > MEM_MIN_OBJECT_SIZE * MEM_MEMORY_POOL_ORDER) {
        printf("[MEMORY][ERROR] can't allocate chunk, size too big");
        return NULL;
    }

    U8 fit_order = size / MEM_MIN_OBJECT_SIZE;

    // if 剛好相等, ex. 16, 32 is order to it's bin
    if ((size - (MEM_MIN_OBJECT_SIZE * fit_order)) == 0)
        fit_order--;

    MEMORY_POOL* order_pool = &mem_manager.pools[fit_order];

    for (U32 i = 0; i < order_pool->chunk_count; i++) {
        // has space for this chunk
        CHUNK_INFO* chunk_info = &order_pool->chunks[i];
        if (chunk_info->ref_count < order_pool->slot_size) {
            for (U32 j = 0; j < order_pool->slot_size; j++) {
                CHUNK_SLOT* slot = &chunk_info->slots[j];
                if (slot->flag & MEM_CHUNK_SLOT_FLAG_USED) {
                    continue;
                }

                // this slot it not use
                slot->flag |= MEM_CHUNK_SLOT_FLAG_USED;
                chunk_info->ref_count++;

                char* return_addr = MEM_INDEX_TO_ADDR(chunk_info->frame_index);
                return_addr = return_addr + (j * order_pool->obj_size);

                NS_DPRINT("[MEMORY][TRACE] chunk slot allocated. index: %d, offset: %d\n", chunk_info->frame_index, j);
                return (void*)return_addr;
            }
        }
    }

    // no chunk can be allocate, allocate chunk
    CHUNK_INFO* new_chunk = &order_pool->chunks[order_pool->chunk_count];
    order_pool->chunk_count++;
    new_chunk->frame_index = mem_buddy_alloc(0);
    new_chunk->ref_count = 1;
    new_chunk->slots[0].flag |= MEM_CHUNK_SLOT_FLAG_USED;
    
    FRAME_INFO* used_frame = &mem_manager.frames[new_chunk->frame_index];
    used_frame->flag |= MEM_FRAME_FLAG_CHUNK;
    used_frame->pool_order = fit_order;

    NS_DPRINT("[MEMORY][TRACE] New chunk allocated\n");
    NS_DPRINT("[MEMORY][TRACE] chunk slot allocated. index: %d, offset: %d\n", new_chunk->frame_index, 0);

    return (void*)MEM_INDEX_TO_ADDR(new_chunk->frame_index);
}

/**
 * A clumsy approach to Linux slab
*/
void mem_chunk_free(void* ptr) {
    U32 frame_index = MEM_ADDR_TO_INDEX(ptr);

    FRAME_INFO* frame_info = &mem_manager.frames[frame_index];

    if (!(frame_info->flag & MEM_FRAME_FLAG_CHUNK)) {
        printf("[MEMORY][WARN] Freeing not chunk memory!\n");
        return;
    }

    MEMORY_POOL* pool = &mem_manager.pools[frame_info->pool_order];

    // searching chunk
    CHUNK_INFO* chunk_info = NULL;
    U8 chunk_offset = 0;
    for(U8 i = 0; i < pool->chunk_count; i++) {
        CHUNK_INFO* search_chunk = &pool->chunks[i];
        if (search_chunk->frame_index == frame_index) {
            chunk_info = search_chunk;
            chunk_offset = i;
            break;
        }
    }

    if (chunk_info == NULL) {
        printf("[MEMORY][ERROR] chunk not found in pool list!\n");
        return;
    }

    char* offset_ptr = (char*)ptr - MEM_INDEX_TO_ADDR(frame_index);
    UPTR slot_offset = (UPTR)offset_ptr / pool->obj_size;

    CHUNK_SLOT* slot = &chunk_info->slots[slot_offset];
    if (!(slot->flag & MEM_CHUNK_SLOT_FLAG_USED)) {
        printf("[MEMORY][ERROR] slot is not used!\n");
        return;
    }
    slot->flag &= ~MEM_CHUNK_SLOT_FLAG_USED;
    chunk_info->ref_count--;

    // chunk has no reference, free chunk
    if (chunk_info->ref_count == 0) {
        mem_buddy_free(chunk_info->frame_index);

        frame_info->flag &= ~MEM_FRAME_FLAG_CHUNK;

        // remove from pool
        for (U8 i = chunk_offset; i < pool->chunk_count - 1; i++) {
            pool->chunks[i] = pool->chunks[i + 1];
        }
        pool->chunk_count--;
    }

    NS_DPRINT("[MEMORY][TRACE] free chunk memory successfully, ptr:%x\n", ptr);
}


void buddy_init() {

    // to fill the free list
    //U64 frame_index = 0;

    // for each level
    NS_DPRINT("[TRACE] init each buddy level\n");
    for (int current_level = mem_manager.levels - 1; current_level >= 0; current_level--) {
        NS_DPRINT("[TRACE] Current level: %d\n", current_level);
        // the block size of this level
        U64 number_of_frames_in_block = (1 << current_level);
        U64 level_size = number_of_frames_in_block * MEM_FRAME_SIZE;

        // calculate the max size of this level free list
        U32 max_free_size = (mem_manager.size / level_size) + 1;
        FREE_INFO* current_info = &mem_manager.free_list[current_level];
        current_info->space = max_free_size;
        current_info->info = smalloc(max_free_size * sizeof(int));
        NS_DPRINT("[MEMORY][TRACE] free list init. level: %d, space: %d\n", current_level, current_info->space);

        for(U32 i = 0;i < current_info->space; i++) {
            current_info->info[i] = MEM_FREE_INFO_UNUSED;
        }
        // initialize the buddy
        /*
        U32 info_index = 0;
        while (frame_index + number_of_frames_in_block <= mem_manager.number_of_frames) {
            current_info->info[info_index] = frame_index;
            
            mem_manager.frames[frame_index].flag = 0;
            mem_manager.frames[frame_index].order = current_level;
            for (U64 i = 1; i < number_of_frames_in_block; i++) {
                mem_manager.frames[frame_index + i].flag = MEM_FRAME_FLAG_CONTINUE;
                mem_manager.frames[frame_index + i].order = current_level;
            }
            info_index++;
            frame_index += number_of_frames_in_block;
        }
        NS_DPRINT("[MEMORY][TRACE] free info insered. count: %d, frame offset: %d\n", info_index, frame_index);
        current_info->size = info_index;
        for(;info_index < current_info->space; info_index++) {
            current_info->info[info_index] = MEM_FREE_INFO_UNUSED;
        }
        */
    }

    U32 frame_offset = 0;
    while (frame_offset < mem_manager.number_of_frames) {

        // first add until hit the usable block region
        U32 usable_offset = 0;
        while (frame_offset + usable_offset < mem_manager.number_of_frames) {
            if (mem_manager.frames[frame_offset + usable_offset].flag & MEM_FRAME_FLAG_CANT_USE)
                break;
            usable_offset++;
        }

        // TODO: put into free list
        U32 region_size = usable_offset;
        while (region_size > 0) {
            U32 order = utils_highestOneBit(region_size) - 1;
            U64 number_of_frames_in_block = (1 << order); 

            FREE_INFO* free_info = &mem_manager.free_list[order];
            free_info->info[free_info->size] = frame_offset;
            free_info->size++;

            // first frame
            mem_manager.frames[frame_offset].flag &= ~(MEM_FRAME_FLAG_USED);
            mem_manager.frames[frame_offset].order = order;
            // rest frame  
            for (U64 i = 1; i < number_of_frames_in_block; i++) {
                mem_manager.frames[frame_offset + i].flag &= ~(MEM_FRAME_FLAG_USED);
                mem_manager.frames[frame_offset + i].flag |= MEM_FRAME_FLAG_CONTINUE;
                mem_manager.frames[frame_offset + i].order = order;
            }
            frame_offset += number_of_frames_in_block;
            region_size -= number_of_frames_in_block;
            NS_DPRINT("[MEMORY][TRACE] free info insered. count: %d, frame offset: %d\n", free_info->size - 1, frame_offset);
        }
        
        // loop until can use region
        while (frame_offset < mem_manager.number_of_frames) {
            if (!(mem_manager.frames[frame_offset].flag & MEM_FRAME_FLAG_CANT_USE))
                break;
            frame_offset++;
        }
    }

}

/***
 * Get the free frame index with certain order
 * will remove from the free list
*/
U32 mem_get_free(U8 order) {
    U64 number_of_frames_in_block = (1 << order);

    FREE_INFO* current_free_info = &mem_manager.free_list[order];
    if (current_free_info->size == 0)
        return MEM_FREE_INFO_UNUSED;
    
    // having enough space in free list
    U32 start_frame = current_free_info->info[current_free_info->size - 1];

    // mark the frames in use
    mem_manager.frames[start_frame].flag |= MEM_FRAME_FLAG_USED;
    mem_manager.frames[start_frame].flag &= ~MEM_FRAME_FLAG_CONTINUE;
    mem_manager.frames[start_frame].order = order;
    for (U64 i = 1; i < number_of_frames_in_block; i++) {
        mem_manager.frames[start_frame + i].flag |= MEM_FRAME_FLAG_USED | MEM_FRAME_FLAG_CONTINUE;
        mem_manager.frames[start_frame + i].order = order;
    }

    // remove from the free list
    current_free_info->info[current_free_info->size - 1] = MEM_FREE_INFO_UNUSED;
    current_free_info->size--;
#ifdef NS_DEBUG
    printf("[MEMORY][DEBUG] Found buddy, order: %d Start frame: %d\n", order, start_frame);
#endif
    return start_frame;
    /** Old method
    for (U32 i = 0; i < current_free_info->space; i++) {
        if (current_free_info->info[i] == MEM_FREE_INFO_UNUSED)
            break;
        
        // having enough space
        U32 start_frame = current_free_info->info[i];

        // mark the frames in use
        mem_manager.frames[start_frame].flag |= MEM_FRAME_FLAG_USED;
        mem_manager.frames[start_frame].flag &= ~MEM_FRAME_FLAG_CONTINUE;
        mem_manager.frames[start_frame].order = order;
        for (U64 i = 1; i < number_of_frames_in_block; i++) {
            mem_manager.frames[start_frame + i].flag |= MEM_FRAME_FLAG_USED | MEM_FRAME_FLAG_CONTINUE;
            mem_manager.frames[start_frame + i].order = order;
        }

        // remove from the free list
        for (U32 j = i + 1; j < current_free_info->space; j++) {
            if (current_free_info->info[j] == MEM_FREE_INFO_UNUSED) {
                current_free_info->info[j - 1] = MEM_FREE_INFO_UNUSED;
                break;
            }
            current_free_info->info[j - 1] = current_free_info->info[j];
        }
        current_free_info->size--;
#ifdef NS_DEBUG
        printf("[MEMORY][DEBUG] Found buddy, order: %d Start frame: %d\n", order, start_frame);
#endif
        return start_frame;
    }
    return MEM_FREE_INFO_UNUSED;
    */
}

/**
*/
int mem_put_free(U32 frame_index, U8 order) {
    U64 number_of_frames_in_block = (1 << order);

    FREE_INFO* current_free_info = &mem_manager.free_list[order];

    FRAME_INFO* frame_info = &mem_manager.frames[frame_index];

    if (current_free_info->size + 1 >= current_free_info->space) {
        printf("[MEMORY][ERROR] Free list out of index! order: %d\n", order);
        printf("[MEMORY][ERROR] current size: %d, space: %d\n", current_free_info->size, current_free_info->space);
        return -1;
    }
    current_free_info->info[current_free_info->size] = frame_index;
    current_free_info->size++;

/** Old method
    U32 blank_info_idx = 0;
    for (; blank_info_idx < current_free_info->space; blank_info_idx++) {
        if (current_free_info->info[blank_info_idx] == MEM_FREE_INFO_UNUSED)
            break;
    }
    if (blank_info_idx >= current_free_info->space) {
        printf("[MEMORY][ERROR] Free list out of index!\n");
        return -1;
    }

    // put the index to the free list
    current_free_info->info[blank_info_idx] = frame_index;
    current_free_info->size++;
*/

    // mark the frames in free
    frame_info->flag &= ~(MEM_FRAME_FLAG_USED | MEM_FRAME_FLAG_CONTINUE);
    frame_info->order = order;
    for (U64 i = 1; i < number_of_frames_in_block; i++) {
        mem_manager.frames[frame_index + i].flag &= ~MEM_FRAME_FLAG_USED;
        mem_manager.frames[frame_index + i].flag |= MEM_FRAME_FLAG_CONTINUE;
        mem_manager.frames[frame_index + i].order = order;
    }

    return 0;
}

/**
 * Buddy allocation
 * @param order
 *      the exp of the alloc memory
 *      ex. 0 = allocate 2^0 = 1 page
 * @return founded first frame index
 *      if return MEM_FRAME_NOT_FOUND mean out of memory
 * 
*/
U32 mem_buddy_alloc(U8 order) {

    // search for current free list
    U32 frame_index = mem_get_free(order);

    // found the frame index for the current order
    if (frame_index != MEM_FREE_INFO_UNUSED) {
        NS_DPRINT("[MEMORY][TRACE] No need to find larger block. index: %d, order: %d\n", frame_index, order);
        return frame_index;
    }

    // not found in current level try to search larger space
    U8 search_order = order + 1;
    BOOL foundSpace = FALSE;
    while (search_order < mem_manager.levels) {
        frame_index = mem_get_free(search_order);

        if (frame_index != MEM_FREE_INFO_UNUSED) {
            foundSpace = TRUE;
            break;
        }

        search_order++;
    }
    if (foundSpace == FALSE) {
        printf("[MEMORY][ERROR] Failed to allocate buddy, out of memory\n");
        return MEM_FRAME_NOT_FOUND;
    }
    NS_DPRINT("[MEMORY][TRACE] Allocating larger block. index: %d, order: %d\n", frame_index, search_order);

    // split the founded buddy
    U8 split_order = search_order;
    // iterate it and put the bottom half back to free list
    while (split_order > order) {
        U32 bottom_index = frame_index + (1 << (split_order - 1));
        mem_put_free(bottom_index, split_order - 1);
        NS_DPRINT("[MEMORY][TRACE] Put redundant memory block back to free list. index: %d, order: %d\n", bottom_index, split_order - 1);
        split_order--;
    }

    // change the order of the current start frame
    mem_manager.frames[frame_index].flag |= MEM_FRAME_FLAG_USED;
    mem_manager.frames[frame_index].flag &= ~MEM_FRAME_FLAG_CONTINUE;
    mem_manager.frames[frame_index].order = order;
    U64 number_of_frames_in_block = (1 << order);
    for (U64 i = 1; i < number_of_frames_in_block; i++) {
        mem_manager.frames[frame_index + i].flag |= MEM_FRAME_FLAG_USED | MEM_FRAME_FLAG_CONTINUE;
        mem_manager.frames[frame_index + i].order = order;
    }

    return frame_index;
}

/**
 * @param order
 *      the order of the larger block
*/
BOOL mem_is_first_buddy(U32 frame_index, U8 order) {
    return (frame_index & ((1 << order) - 1)) == 0;
}

void mem_buddy_free(U32 frame_index) {

    // check if it is free space
    if (!(mem_manager.frames[frame_index].flag & MEM_FRAME_FLAG_USED)) {
        printf("[MEMORY][ERROR] This block already free. index: %d, order: %d\n", frame_index, mem_manager.frames[frame_index].order);
        return;
    }

    U8 order = mem_manager.frames[frame_index].order;

    U8 check_order = order + 1;

    while (check_order < mem_manager.levels) {
        U32 other_buddy;
        BOOL current_frame_first = mem_is_first_buddy(frame_index, check_order);
        // check whether it is the first buddy
        if (current_frame_first) {
            other_buddy = frame_index + (1 << (check_order - 1));
        } else {
            other_buddy = frame_index - (1 << (check_order - 1));
        }

        if (other_buddy >= mem_manager.number_of_frames) {
            NS_DPRINT("[MEMORY][TRACE] the other buddy is not in memory, index: %d\n", other_buddy);
            break;
        }

        if (mem_manager.frames[other_buddy].flag & MEM_FRAME_FLAG_CANT_USE) {
            NS_DPRINT("[MEMORY][TRACE] buddy partner can not be use (reserved memory). buddy: %d\n", other_buddy);
            break;
        }

        // buddy is not free, can't merge
        if (MEM_FRAME_IS_INUSE(mem_manager.frames[other_buddy])) {
            NS_DPRINT("[MEMORY][TRACE] buddy partner is not free. buddy: %d\n", other_buddy);
            break;
        }
        
        if (mem_manager.frames[other_buddy].order != check_order - 1) {
            NS_DPRINT("[MEMORY][TRACE] buddy partner is free but not the same order\n");
            break;
        }
        // buddy is free, merge
        NS_DPRINT("[MEMORY][TRACE] buddy partner is free and match start merging\n");
        NS_DPRINT("[MEMORY][TRACE] buddy partner index: %d, order: %d\n", other_buddy, check_order - 1);

        // remove the buddy partner from free list
        FREE_INFO* buddy_order_free_list = &mem_manager.free_list[order];
        U32 i = 0;
        for (; i < buddy_order_free_list->space; i++) {
            if (buddy_order_free_list->info[i] == other_buddy) {
                for (U32 j = i + 1; j < buddy_order_free_list->space; j++) {
                    if (buddy_order_free_list->info[j] == MEM_FREE_INFO_UNUSED) {
                        buddy_order_free_list->info[j - 1] = MEM_FREE_INFO_UNUSED;
                        break;
                    }
                    buddy_order_free_list->info[j - 1] = buddy_order_free_list->info[j];
                }
                buddy_order_free_list->size--;
                break;
            }
        }
#ifdef NS_DEBUG
        if (i == buddy_order_free_list->space) {
            printf("[MEMORY][ERROR] Wired, buddy partner is not in free list. buddy index: %d\n", other_buddy);
        }
#endif

        if (!current_frame_first) {
            frame_index = other_buddy;
        }
        order++;
        check_order++;
    }

    // put the buddy to the free list
    FREE_INFO* free_list = &mem_manager.free_list[order];
    U32 i = free_list->size;
    for (; i < free_list->space; i++) {
        if (free_list->info[i] == MEM_FREE_INFO_UNUSED) {
            free_list->info[i] = frame_index;
            free_list->size++;
            break;
        }
    }
#ifdef NS_DEBUG
    if (i == free_list->size) {
        printf("[MEMORY][ERROR] Not put the free buddy into list.\n");
    }
#endif

    mem_manager.frames[frame_index].flag &= ~(MEM_FRAME_FLAG_USED | MEM_FRAME_FLAG_CONTINUE);
    mem_manager.frames[frame_index].order = order;
    U64 number_of_frames_in_block = (1 << order);
    for (U64 i = 1; i < number_of_frames_in_block; i++) {
        mem_manager.frames[frame_index + i].flag &= ~MEM_FRAME_FLAG_USED;
        mem_manager.frames[frame_index + i].flag |= MEM_FRAME_FLAG_CONTINUE;
        mem_manager.frames[frame_index + i].order = order;
    }
    NS_DPRINT("[MEMORY][TRACE] buddy free. index:%d, order: %d\n", frame_index, order);

}

void mem_memory_reserve(UPTR start, UPTR end) {

    U32 start_frame = MEM_ADDR_TO_INDEX(start);
    U32 end_frame = MEM_ADDR_TO_INDEX(end);

    for(U32 i = start_frame; i <= end_frame; i++) {
        mem_manager.frames[i].flag |= MEM_FRAME_FLAG_CANT_USE;
    }

}

void* kmalloc(U64 size) {
    
    if (size > MEM_MIN_OBJECT_SIZE * MEM_MEMORY_POOL_ORDER) {
        U8 order = (U8)utils_highestOneBit(size);
        return MEM_INDEX_TO_ADDR(mem_buddy_alloc(order));
    }

    return mem_chunk_alloc(size);
}

void kfree(void* ptr) {
    U32 frame_index = MEM_ADDR_TO_INDEX(ptr);
    if (mem_manager.frames[frame_index].flag & MEM_FRAME_FLAG_CHUNK) {
        mem_chunk_free(ptr);
    } else {
        mem_buddy_free(frame_index);
    }
}
