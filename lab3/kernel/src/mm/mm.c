
#include <math.h>

#include "mm/mm.h"
#include "io/dtb.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "peripherals/mailbox.h"

extern char __static_mem_start__, __static_mem_end__;

void buddy_init();

// the base of memory

MEMORY_MANAGER mem_manager;

#define MEM_ADDR_TO_INDEX(x)    ((((UPTR)x & -MEM_FRAME_SIZE) - mem_manager.base_ptr) / MEM_FRAME_SIZE)
#define MEM_INDEX_TO_ADDR(x)    (U64*)((x * MEM_FRAME_SIZE) + mem_manager.base_ptr)

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

/**
 * Initialize this trash os memory management
*/
void mm_init() {
    // initialize the static malloc
    smalloc_ptr = (U64*)&__static_mem_start__;
    smalloc_end_ptr = (U64*)&__static_mem_end__;

    // use the example base offset (or use the kernel end ptr)
    mem_manager.base_ptr = 0x10000000;

    // get the memory size
    get_arm_mem();
    if (mailbox_call()) {
        //mem_manager.base_ptr = mailbox[5];
        // end - start
        mem_manager.size = mailbox[6] - mem_manager.base_ptr;
    } else {
        printf("Unable to query memory info!\n");
    }

    buddy_init();
}

void buddy_init() {

    mem_manager.number_of_frames = mem_manager.size / MEM_FRAME_SIZE;
    mem_manager.frames = smalloc(mem_manager.number_of_frames * sizeof(MEM_FRAME_INFO_SIZE));

    mem_manager.levels = utils_highestOneBit(mem_manager.number_of_frames);

    mem_manager.free_list = smalloc(mem_manager.levels * sizeof(FREE_INFO));

    // to fill the free list
    U64 frame_index = 0;

    // for each level
    NS_DPRINT("[TRACE] init each buddy level\n");
    for (U32 current_level = mem_manager.levels - 1; current_level > 0; current_level--) {
        NS_DPRINT("[TRACE] Current level: %d\n", current_level);
        // the block size of this level
        U64 number_of_frames_in_block = (1 << current_level);
        U64 level_size = number_of_frames_in_block * MEM_FRAME_SIZE;

        // calculate the max size of this level free list
        U32 max_free_size = (mem_manager.size / level_size) + 1;
        FREE_INFO* current_info = &mem_manager.free_list[current_level];
        current_info->size = max_free_size;
        current_info->info = smalloc(max_free_size * sizeof(int));

        // initialize the buddy
        U32 info_index = 0;
        while ((mem_manager.base_ptr + (frame_index * MEM_FRAME_SIZE)) + level_size < mem_manager.base_ptr + mem_manager.size) {
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
        for(;info_index < current_info->size; info_index++) {
            current_info->info[info_index] = MEM_FREE_INFO_UNUSED;
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
    for (U32 i = 0; i < current_free_info->size; i++) {
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
        for (U32 j = i + 1; j < current_free_info->size; j++) {
            if (current_free_info->info[j] == MEM_FREE_INFO_UNUSED) {
                current_free_info->info[j - 1] = MEM_FREE_INFO_UNUSED;
                break;
            }
            current_free_info->info[j - 1] = current_free_info->info[j];
        }
#ifdef NS_DEBUG
        printf("[MEMORY][DEBUG] Found buddy, order: %d Start frame: %d\n", order, start_frame);
#endif
        return start_frame;
    }

    return -1;
}

/**
*/
int mem_put_free(U32 frame_index, U8 order) {
    U64 number_of_frames_in_block = (1 << order);

    FREE_INFO* current_free_info = &mem_manager.free_list[order];

    FRAME_INFO* frame_info = &mem_manager.frames[frame_index];

    U32 blank_info_idx = 0;
    for (; blank_info_idx < current_free_info->size; blank_info_idx++) {
        if (current_free_info->info[blank_info_idx] == MEM_FREE_INFO_UNUSED)
            break;
    }
    if (blank_info_idx >= current_free_info->size) {
        printf("[MEMORY][ERROR] Free list out of index!\n");
        return -1;
    }

    // put the index to the free list
    current_free_info->info[blank_info_idx] = frame_index;

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
#ifdef NS_DEBUG
        printf("[MEMORY][TRACE] No need to find larger block. index: %d, order: %d\n", frame_index, order);
#endif
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
#ifdef NS_DEBUG
        printf("[MEMORY][TRACE] Allocating larger block. index: %d, order: %d\n", frame_index, search_order - 1);
#endif

    // split the founded buddy
    U8 split_order = search_order;
    // iterate it and put the bottom half back to free list
    while (split_order > order) {
        U32 bottom_index = frame_index + (1 << (split_order - 1));
        mem_put_free(bottom_index, split_order - 1);
#ifdef NS_DEBUG
        printf("[MEMORY][TRACE] Put redundant memory block index: %d, order: %d\n", bottom_index, split_order - 1);
#endif
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

        // buddy is not free, can't merge
        if (mem_manager.frames[other_buddy].flag & MEM_FRAME_FLAG_USED) {
#ifdef NS_DEBUG
            printf("[MEMORY][TRACE] buddy partner is not free\n");
#endif
            break;
        }
        
        if (mem_manager.frames[other_buddy].order != check_order - 1) {
#ifdef NS_DEBUG
            printf("[MEMORY][TRACE] buddy partner is free but not the same order\n");
#endif
            break;
        }
        // buddy is free, merge
        NS_DPRINT("[MEMORY][TRACE] buddy partner is free and match start merging\n");
        NS_DPRINT("[MEMORY][TRACE] buddy partner index: %d, order: %d\n", other_buddy, check_order - 1);

        // remove the buddy from free list
        FREE_INFO* buddy_order_free_list = &mem_manager.free_list[order];
        U32 i = 0;
        for (; i < buddy_order_free_list->size; i++) {
            if (buddy_order_free_list->info[i] == other_buddy) {
                for (U32 j = i + 1; j < buddy_order_free_list->size; j++) {
                    if (buddy_order_free_list->info[j] == MEM_FREE_INFO_UNUSED) {
                        buddy_order_free_list->info[j - 1] = MEM_FREE_INFO_UNUSED;
                        break;
                    }
                    buddy_order_free_list->info[j - 1] = buddy_order_free_list->info[j];
                }
                break;
            }
        }
#ifdef NS_DEBUG
        if (i == buddy_order_free_list->size) {
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
    U32 i = 0;
    for (; i < free_list->size; i++) {
        if (free_list->info[i] == MEM_FREE_INFO_UNUSED) {
            free_list->info[i] = frame_index;
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

/**
 * @param pages
 *      the number of pages you want to allocate
*/
void* alloc_pages(U64 pages) {
    if (pages == 0)
        return NULL;

    // TODO:
    return NULL;
}