#include "buddy_system.h"
#include "alloc.h"
#include "../peripherals/utils.h"
#include "../peripherals/mini_uart.h"

list_header* free_block_list[FREE_BLOCK_LIST_SIZE];

// Store the info of allocated memory.
allocate_info* alloc_mem_list[MAX_ALLOC];

// Store the info of reserved memory.
reserve_mem_list* rsv_mem_head;

// Size of free memory available.
uint64_t free_mem_size;

void init_frame_freelist(void) {
    free_mem_size = FREE_MEM_END_ADDR - FREE_MEM_BASE_ADDR;
    if (free_mem_size < get_chunk_size(FREE_BLOCK_LIST_SIZE - 1)) {
        uart_send_string("Not enough free memory! Increase free memory size or lower maximum block size!\r\n");
        return;
    }

    // Initialize list headers.
    for (int i = 0; i < FREE_BLOCK_LIST_SIZE; i++) {
        list_header* new_header = (list_header *)malloc(sizeof(list_header));

        new_header->next_block = NULL;

        free_block_list[i] = new_header;
    }

    // Initialize the list storing allocated frames.
    for (int i = 0; i < MAX_ALLOC; i++) {
        alloc_mem_list[i] = NULL;
    }

    // Initialize the empty list with maximum chunk size(free_list_tail).
    int chunk_cnt = free_mem_size / get_chunk_size(FREE_BLOCK_LIST_SIZE - 1);

    free_block* cur_block;
    for (int i = 0; i < chunk_cnt; i++) {
        free_block* new_block = (free_block *)malloc(sizeof(free_block));
        
        if (i == 0) {
            new_block->ind = 0;
            new_block->next = NULL;
            new_block->prev = new_block;
            free_block_list[FREE_BLOCK_LIST_SIZE - 1]->next_block = new_block;
            cur_block = new_block;
        } else {
            new_block->ind = cur_block->ind + power(2, FREE_BLOCK_LIST_SIZE - 1);
            new_block->next = NULL;
            new_block->prev = cur_block;
            cur_block->next = new_block;
            cur_block = cur_block->next;
        }
    }

}

allocate_info* allocate_frame(uint64_t request_size) {
    free_block* allocate_block = NULL;
    allocate_info* info = NULL;

    for (int i = 0; i < FREE_BLOCK_LIST_SIZE; i++) {

        // List empty.
        if (free_block_list[i]->next_block == NULL) {
            continue;
        }
        
        // Each chunk size in current list.
        uint64_t cur_chunk_size = get_chunk_size(i);
        
        if (request_size <= cur_chunk_size) {
            info = (allocate_info *)malloc(sizeof(allocate_info));

            // The starting block of the returned list.
            allocate_block = free_block_list[i]->next_block;
            info->start_frame = allocate_block->ind;
            info->last_frame = info->start_frame + (cur_chunk_size / BLOCK_SIZE) - 1;

            uart_send_string("\r\n---------------------------------------\r\n");
            uart_send_string("Allocate Memory Info\r\n");
            uart_send_string("---------------------------------------\r\n");
            uart_send_string("Allocate block of memory: ");
            uart_send_int(cur_chunk_size / 1024);
            uart_send_string(" KB, starting from frame ");
            uart_send_int(allocate_block->ind);
            uart_send_string("\r\n\r\n");


            free_block_list[i]->next_block = allocate_block->next;

            if (free_block_list[i]->next_block != NULL) {
                free_block_list[i]->next_block->prev = free_block_list[i]->next_block;
            }
            
            // Calculate the remaining memory of allocated block.
            uint64_t rem_mem = cur_chunk_size - request_size;

            // The last chunk still has memory left.
            if (rem_mem > 0) {
                info->last_frame = release_block(allocate_block->ind, cur_chunk_size, request_size);
            }

            insert_alloc_list(info);

            break;
        }
    }

    check_list();

    return info;
}

// Put back remaining block after allocating and return the index of the last allocated block.
int release_block(int start_ind, uint64_t chunk_size, uint64_t used_mem) {

    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Release Remaining Memory Info\r\n");
    uart_send_string("---------------------------------------\r\n");

    while (chunk_size > BLOCK_SIZE) {
        // The last half is free to put back.
        if (chunk_size / 2 >= used_mem) {
            int exp = find_chunk_exp(chunk_size / 2);
            int ind = start_ind + (chunk_size / 2) / BLOCK_SIZE;

            uart_send_string("Release ");
            uart_send_int(chunk_size / 2 / 1024);
            uart_send_string(" KB memory, starting from frame ");
            uart_send_int(ind);
            uart_send_string("\r\n");

            free_block* cur_block = free_block_list[exp]->next_block;
            free_block* new_block = (free_block *)malloc(sizeof(free_block));

            new_block->ind = ind;
            new_block->prev = new_block;
            new_block->next = NULL;

            // Insert the last half into free list.
            if (cur_block == NULL) {
                free_block_list[exp]->next_block = new_block;
            } else {
                while (cur_block != NULL) {
                    // Insert free chunk in increasing order.
                    if (cur_block->ind > ind) {
                        // New block should be head of list.
                        if (cur_block->prev == cur_block) {
                            free_block_list[exp]->next_block = new_block;
                            new_block->next = cur_block;
                            cur_block->prev = new_block;
                        } else {
                            cur_block->prev->next = new_block;
                            new_block->prev = cur_block->prev;
                            new_block->next = cur_block;
                            cur_block->prev = new_block;
                        }
                        break;
                    // New block should be inserted at the tail.
                    } else if (cur_block->next == NULL) {
                        cur_block->next = new_block;
                        new_block->prev = cur_block;
                        break;
                    }
                    cur_block = cur_block->next;
                }
            }
            
            chunk_size /= 2;

        } else {
            chunk_size /= 2;
            used_mem -= chunk_size;
            start_ind += (chunk_size / BLOCK_SIZE);
        }
    }

    uart_send_string("\r\n");
    
    return start_ind;
}

// Release the block and put it back to the free list, also updating the allocated
// memory list.
void free_allocated_mem(int start_ind) {
    int start_frame;
    int last_frame;

    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Free Memory Info\r\n");
    uart_send_string("---------------------------------------\r\n");

    for (int i = 0; i < MAX_ALLOC; i++) {
        if (alloc_mem_list[i] != NULL && alloc_mem_list[i]->start_frame == start_ind) {
            start_frame = alloc_mem_list[i]->start_frame;
            last_frame = alloc_mem_list[i]->last_frame;
            alloc_mem_list[i] = NULL;
            break;
        }

        if (i == MAX_ALLOC - 1) {
            uart_send_string("Allocated memory not found!\r\n");
            return;
        }
    }

    unsigned long size = (last_frame - start_frame + 1) * (BLOCK_SIZE / 1024);
    
    while (size > 0) {
        for (int i = 0; i < FREE_BLOCK_LIST_SIZE; i++) {
            if (size >= get_chunk_size(i) / 1024 && size < get_chunk_size(i + 1) / 1024) {
                uart_send_string("start_frame: ");
                uart_send_int(start_frame);
                uart_send_string("\r\n");
                uart_send_string("list_index: ");
                uart_send_int(i);
                uart_send_string("\r\n");
                merge_buddy(start_frame, i);
                size -= get_chunk_size(i) / 1024;
                start_frame += (get_chunk_size(i) / BLOCK_SIZE);
            }
        }
    }


}

void insert_alloc_list(allocate_info* info) {
    for (int i = 0; i < MAX_ALLOC; i++) {
        if (alloc_mem_list[i] == NULL) {
            alloc_mem_list[i] = info;

            return;
        }
    }

    uart_send_string("Maximum allocation reached! Failed to allocated memory!\r\n");
}

void show_alloc_list2(void) {
    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Allocated Memory Info\r\n");
    uart_send_string("---------------------------------------\r\n");

    for (int i = 0; i < MAX_ALLOC; i++) {
        if (alloc_mem_list[i] != NULL) {
            uart_send_string("Allocated frames, starting from frame ");
            uart_send_int(alloc_mem_list[i]->start_frame);
            uart_send_string(" to frame ");
            uart_send_int(alloc_mem_list[i]->last_frame);
            uart_send_string("\r\n");
        }
    }
    uart_send_string("\r\n");
}

uint64_t get_chunk_size(int list_ind) {
    return BLOCK_SIZE * power(2, list_ind);
}

int find_chunk_exp(uint64_t size) {
    int tmp = size / BLOCK_SIZE;
    int exp = 0;

    while (tmp > 0) {
        exp++;
        tmp /= 2;
    }

    exp--;

    return exp;
}

int find_bud(int ind, int exp) {
    return ind ^ (1 << exp);
}

void merge_buddy(int ind, int free_list_ind) {
    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("In merge_buddy\r\n");
    uart_send_string("---------------------------------------\r\n");
    
    uart_send_string("Finding buddy of block ");
    uart_send_int(ind);
    uart_send_string(" in list ");
    uart_send_int(free_list_ind);
    uart_send_string("\r\n");

    free_block* cur_block = free_block_list[free_list_ind]->next_block;

    // Maximum size, don't merge.
    if (free_list_ind == FREE_BLOCK_LIST_SIZE - 1) {
        uart_send_string("Maximum block size, no need to merge.\r\n");
        free_block* new_block = (free_block *)malloc(sizeof(free_block));
        new_block->ind = ind;
        if (cur_block == NULL) {
            new_block->next = NULL;
            new_block->prev = new_block;
            free_block_list[free_list_ind]->next_block = new_block;
        } else {
            while (cur_block != NULL) {
                if (cur_block->ind > ind) {
                    // cur_block is head.
                    if (cur_block->prev = cur_block) {
                        free_block_list[free_list_ind]->next_block = new_block;
                        new_block->next = cur_block;
                        cur_block->prev = new_block;
                    } else {
                        cur_block->prev->next = new_block;
                        new_block->prev = cur_block->prev;
                        new_block->next = cur_block;
                        cur_block->prev = new_block;
                    }
                    break;
                }
                cur_block = cur_block->next;
            }
        }

        return;
    }

    int buddy_ind = find_bud(ind, free_list_ind);

    // Find whether buddy exists.
    while (cur_block != NULL) {
        if (cur_block->ind == buddy_ind) {
            uart_send_string("Buddy found ---> Block ");
            uart_send_int(buddy_ind);
            uart_send_string("\r\n");
            // Remove the block from the current list and merge it to larger memory list.
            // If the block is the head.
            if (cur_block->prev == cur_block) {
                free_block_list[free_list_ind]->next_block = cur_block->next;
                if (cur_block->next != NULL) {
                    cur_block->next->prev = cur_block->next;
                }
            // Block is tail.
            } else if (cur_block->next == NULL) {
                cur_block->prev->next = NULL;
            // Block is at the middle
            } else {
                cur_block->prev->next = cur_block->next;
                cur_block->next->prev = cur_block->prev;
            }

            if (buddy_ind > ind) {
                merge_buddy(ind, free_list_ind + 1);
            } else {
                merge_buddy(buddy_ind, free_list_ind + 1);
            }

            return;
        }
        cur_block = cur_block->next;
    }

    uart_send_string("No buddy found!\r\n");

    // No buddy is found, so insert the free block to current list.
    free_block* new_block = (free_block *)malloc(sizeof(free_block));
    new_block->ind = ind;
    new_block->next = NULL;
    new_block->prev = new_block;

    cur_block = free_block_list[free_list_ind]->next_block;
    if (cur_block == NULL) {
        free_block_list[free_list_ind]->next_block = new_block;
    } else {
        while (cur_block != NULL) {
            if (cur_block->ind > new_block->ind) {
                // cur_block is head.
                if (cur_block->prev = cur_block) {
                    cur_block->prev = new_block;
                    new_block->next = cur_block;
                    free_block_list[free_list_ind]->next_block = new_block;
                } else {
                    cur_block->prev->next = new_block;
                    new_block->prev = cur_block->prev;
                    new_block->next = cur_block;
                    cur_block->prev = new_block;
                }
                break;
            } else if (cur_block->next == NULL) {
                cur_block->next = new_block;
                new_block->prev = cur_block;
                break;
            }

            cur_block = cur_block->next;
        }
    }
}

void check_list(void) {

    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Free Frame List Info\r\n");
    uart_send_string("---------------------------------------\r\n");

    for (int i = 0; i < FREE_BLOCK_LIST_SIZE; i++) {
        uart_send_string("List: ");
        uart_send_int(i);
        uart_send_string("\r\n");

        free_block* head = free_block_list[i]->next_block;
        uart_send_string("Free block index: ");
        while (i != FREE_BLOCK_LIST_SIZE - 1 && head != NULL) {
            uart_send_int(head->ind);
            uart_send_string(" ");
            head = head->next;
        }

        if (i == FREE_BLOCK_LIST_SIZE - 1) {
            // Only print ten index or the output will be too much.
            for (int i = 0; i < 10; i++) {
                uart_send_int(head->ind);
                uart_send_string(" ");
                head = head->next;
            }
        }
        uart_send_string("\r\n");

    }
    uart_send_string("\r\n");
}

void init_rsv_mem_list(void) {
    rsv_mem_head = NULL;
}

// Reserve the memory block.(From start to end)
/*
void memory_reserve(uint64_t start, uint64_t end) {
    int start_ind = start / BLOCK_SIZE;
    int end_ind = end / BLOCK_SIZE;

    reserve_mem_list* new = (reserve_mem_list *)malloc(sizeof(reserve_mem_list));
    allocate_info* info = (allocate_info *)malloc(sizeof(allocate_info));
    info->start_frame = start_ind;
    info->last_frame = end_ind;
    new->info = info;
    new->next = NULL;
    new->prev = NULL;

    // The list should be in acsending order according to starting frame index.
    if (rsv_mem_head == NULL) {
        rsv_mem_head = new;
    } else {
        reserve_mem_list* cur = rsv_mem_head;

        while (cur != NULL) {
            if (new->info->start_frame < cur->info->start_frame) {
                if (cur == rsv_mem_head) {
                    new->next = cur;
                    cur->prev = new;
                    rsv_mem_head = new;
                } else {
                    cur->prev->next = new;
                    new->prev = cur->prev;
                    new->next = cur;
                    cur->prev = new;
                }
                break;
            } else if (cur->next == NULL) {
                cur->next = new;
                new->prev = cur;
                break;
            }

            cur = cur->next;
        }
    }


    reserve_mem_list* cur = rsv_mem_head;
    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Reserved Memory\r\n");
    uart_send_string("---------------------------------------\r\n");

    while (cur != NULL) {
        uart_send_string("frame ");
        uart_send_int(cur->info->start_frame);
        uart_send_string(" ~ ");
        uart_send_string("frame ");
        uart_send_int(cur->info->last_frame);
        uart_send_string("\r\n");
        cur = cur->next;
    }
}
*/