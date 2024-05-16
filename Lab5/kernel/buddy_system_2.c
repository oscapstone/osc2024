#include "buddy_system_2.h"
#include "alloc.h"
#include "../peripherals/utils.h"
#include "../peripherals/mini_uart.h"

entry_t** frame_arr;
list_header_t* free_list[FREE_LIST_SIZE];
alloc_header_t* alloc_frame_list[MAX_ALLOC_CNT];

void init_buddy_system(void) {
    uint64_t tot_mem_size = FREE_MEM_END_ADDR - FREE_MEM_BASE_ADDR;
    int frame_cnt = tot_mem_size / FRAME_SIZE;
    uart_send_string("frame_cnt: ");
    uart_send_int(frame_cnt);
    uart_send_string("\r\n");

    for (int i = 0; i < MAX_ALLOC_CNT; i++) {
        alloc_header_t* header = (alloc_header_t *)malloc(sizeof(alloc_header_t));
        header->head = NULL;
        header->tail = NULL;
        header->status = -1;
        alloc_frame_list[i] = header;
    }

    // Check whether memory is divisible by maximum frame size.
    if (tot_mem_size % (FRAME_SIZE * power(2, FREE_LIST_SIZE - 1)) != 0) {
        uart_send_string("Free memory not divisible by maximum frame size!\r\n"); 
        uart_send_string("Buddy system initialization failed!\r\n");
        return;
    }

    frame_arr = (entry_t **)malloc(sizeof(entry_t *) * frame_cnt);

    for (int i = 0; i < FREE_LIST_SIZE; i++) {
        list_header_t* header = (list_header_t *)malloc(sizeof(list_header_t));
        header->head = NULL;
        header->tail = NULL;
        free_list[i] = header;
    }

    int largest_page_size = power(2, FREE_LIST_SIZE - 1);
    entry_t* cur_entry;

    for (int i = 0; i < frame_cnt; i++) {
        entry_t* new_entry = (entry_t *)malloc(sizeof(entry_t));
        new_entry->slot_size = SLOT_NOT_ALLOCATED;
        new_entry->status = FREE_FRAME;
        new_entry->next = NULL;
        new_entry->prev = NULL;
        new_entry->next_slot = NULL;
        new_entry->prev_slot = NULL;
        new_entry->ind = i;
        frame_arr[i] = new_entry;
    }

    // Reserve spin tables for multicore boot.
    memory_reserve(0x0, 0x1000);
    // Reserve kernel image.
    memory_reserve(0x80000, 0xC298390);
    // Reserve cpio.
    memory_reserve(0x10000000, 0x10000400);
    // Reserve dtb.
    memory_reserve(0x2EFF7600, 0x2EFFFFDC);


    for (int i = 0; i < frame_cnt; i++) {
        // uart_send_string("i: ");
        // uart_send_int(i);
        // uart_send_string("\r\n");

        int start_frame = i;
        int contiguous_frames = 0;

        while (i < frame_cnt && frame_arr[i]->status != USED_FRAME) {
            // uart_send_string("In while: ");
            // uart_send_int(i);
            // uart_send_string("\r\n");
            contiguous_frames++;
            i++;

            if (i % 64 == 63) {
                if (frame_arr[i]->status != USED_FRAME) {
                    contiguous_frames++;
                }
                break;
            }
        }



        if (contiguous_frames > 0) {
            insert_freelist(start_frame, contiguous_frames);
        }
    }




    /*
    // Frame index indicating the beginning of the largest continuous block.(256KB)
    if (i % largest_page_size == 0) {
        new_entry->status = FREE_LIST_SIZE - 1;
        new_entry->prev = new_entry;
        new_entry->next = NULL;

        // Head and tail points to the same entry if only one exists.
        if (free_list[FREE_LIST_SIZE - 1]->head == NULL) {
            free_list[FREE_LIST_SIZE - 1]->head = new_entry;
            free_list[FREE_LIST_SIZE - 1]->tail = new_entry;
            cur_entry = new_entry;
        } else {
            cur_entry->next = new_entry;
            new_entry->prev = cur_entry;
            cur_entry = cur_entry->next;
            free_list[FREE_LIST_SIZE - 1]->tail = new_entry;
        }
    } else {
        new_entry->status = FREE_FRAME;
        new_entry->next = NULL;
        new_entry->prev = NULL;
    }
    */

    // show_free_list();
}

void insert_freelist(int start_frame, int contiguous_frames) {

    while (contiguous_frames > 0) {
        // The start frame index has to be divisible by 2^list_index to ensure alignment.
        // 256 KB.
        if (start_frame % (1 << 6) == 0 && contiguous_frames >= (1 << 6)) {
            frame_arr[start_frame]->status = 6;
        // 128 KB.
        } else if (start_frame % (1 << 5) == 0 && contiguous_frames >= (1 << 5)) {
            frame_arr[start_frame]->status = 5;
        // 64 KB.
        } else if (start_frame % (1 << 4) == 0 && contiguous_frames >= (1 << 4)) {
            frame_arr[start_frame]->status = 4;
        // 32 KB.
        } else if (start_frame % (1 << 3) == 0 && contiguous_frames >= (1 << 3)) {
            frame_arr[start_frame]->status = 3;
        // 16 KB.
        } else if (start_frame % (1 << 2) == 0 && contiguous_frames >= (1 << 2)) {
            frame_arr[start_frame]->status = 2;
        // 8 KB.
        } else if (start_frame % (1 << 1) == 0 && contiguous_frames >= (1 << 1)) {
            frame_arr[start_frame]->status = 1;
        // 4 KB.
        } else if (start_frame % (1 << 0) == 0 && contiguous_frames >= (1 << 0)) {
            frame_arr[start_frame]->status = 0;
        }
        
        if (free_list[frame_arr[start_frame]->status]->tail == NULL) {
            free_list[frame_arr[start_frame]->status]->head = frame_arr[start_frame];
            free_list[frame_arr[start_frame]->status]->tail = frame_arr[start_frame];
            frame_arr[start_frame]->prev = frame_arr[start_frame];
            frame_arr[start_frame]->next = NULL;
        } else {
            free_list[frame_arr[start_frame]->status]->tail->next = frame_arr[start_frame];
            frame_arr[start_frame]->prev = free_list[frame_arr[start_frame]->status]->tail;
            frame_arr[start_frame]->next = NULL;
            free_list[frame_arr[start_frame]->status]->tail = frame_arr[start_frame];
        }

        contiguous_frames -= (1 << frame_arr[start_frame]->status);
        start_frame += (1 << frame_arr[start_frame]->status);
    }
}



// Request in bytes.
// Return the index of alloc_frame_list array for the allocation.
int alloc_page(uint64_t request_size) {

    // Uncomment for demo.
    // uart_send_string("\r\n---------------------------------------\r\n");
    // uart_send_string("Allocate Page\r\n");
    // uart_send_string("---------------------------------------\r\n");

    if (request_size > power(2, FREE_LIST_SIZE - 1) * FRAME_SIZE) {
        uart_send_string("Request exceed maximum page size! Allocation failed!\r\n");
        return -1;
    }

    // Find the smallest page required.
    int list_ind = -1;

    for (int i = 0; i < FREE_LIST_SIZE; i++) {
        if (power(2, i) * FRAME_SIZE >= request_size) {
            // The list has free memory.
            if (free_list[i]->head != NULL) {
                list_ind = i;
                break;
            }
        }
    }

    // Uncomment for demo.
    // uart_send_string("Allocate page of size ");
    // uart_send_int(power(2, list_ind) * 4);
    // uart_send_string(" KB\r\n");

    if (list_ind == -1) {
        uart_send_string("No available memory in buddy system!\r\n");
        return -1;
    }

    entry_t* cur_entry = free_list[list_ind]->head;

    // Remove current entry from free list.
    if (free_list[list_ind]->head->next != NULL) {
        free_list[list_ind]->head = free_list[list_ind]->head->next;
    } else {
        free_list[list_ind]->head = NULL;
        free_list[list_ind]->tail = NULL;
    }

    // Release redundant memory.
    if (list_ind != 0) {
        uint64_t prev_list_size = power(2, list_ind - 1) * FRAME_SIZE;

        // Allocated memory is much larger than required. Put back the last half until
        // remaining memory is of correct size.
        while (prev_list_size >= request_size) {
            // Split the list starting from frame <split_ind>.
            int split_ind = cur_entry->ind + power(2, list_ind - 1);
            
            // Insert into the end of free list.
            if (free_list[list_ind - 1]->tail == NULL) {
                free_list[list_ind - 1]->head = frame_arr[split_ind];
                free_list[list_ind - 1]->tail = free_list[list_ind - 1]->head;

                frame_arr[split_ind]->prev = frame_arr[split_ind];
                frame_arr[split_ind]->next = NULL;
            } else {
                free_list[list_ind - 1]->tail->next = frame_arr[split_ind];
                frame_arr[split_ind]->prev = free_list[list_ind - 1]->tail;
                frame_arr[split_ind]->next = NULL;
                free_list[list_ind - 1]->tail = frame_arr[split_ind];
            }

            // Update frame array.
            frame_arr[split_ind]->status = list_ind - 1;
            // uart_send_string("split_ind: ");
            // uart_send_int(split_ind);
            // uart_send_string("\r\n");

            // uart_send_string("f_split_ind: ");
            // uart_send_int(frame_arr[split_ind]->status);
            // uart_send_string("\r\n");
            list_ind--;

            if (list_ind <= 0) {
                break;
            }

            prev_list_size = power(2, list_ind - 1) * FRAME_SIZE;
        }

        // show_free_list();

        cur_entry->status = list_ind;
        // uart_send_string("cur_entry->status: ");
        // uart_send_int(cur_entry->status);
        // uart_send_string("\r\n");
    }

    // Record allocated info.
    int alloc_ind;
    for (alloc_ind = 0; alloc_ind < MAX_ALLOC_CNT; alloc_ind++) {
        if (alloc_frame_list[alloc_ind]->head == NULL) {
            alloc_frame_list[alloc_ind]->head = cur_entry;
            alloc_frame_list[alloc_ind]->tail = frame_arr[cur_entry->ind + power(2, list_ind) - 1];
            alloc_frame_list[alloc_ind]->status = cur_entry->status;
            break;
        }
    }

    if (alloc_ind == MAX_ALLOC_CNT) {
        uart_send_string("Maximum allocation reached! Unable to allocate new frames!\r\n");
        return -1;
    }

    // Update the status of allocated frames.
    // uart_send_string("\r\ncur_entry->ind: ");
    // uart_send_int(cur_entry->ind);
    // uart_send_string("\r\n");
    // uart_send_string("cur_entry->status: ");
    // uart_send_int(power(2, cur_entry->status));
    // uart_send_string("\r\n");
    for (int i = alloc_frame_list[alloc_ind]->head->ind; i <= alloc_frame_list[alloc_ind]->tail->ind; i++) {
        frame_arr[i]->status = USED_FRAME;
    }

    return alloc_ind;
}

void free_page(int start_frame_ind) {

    // uart_send_string("\r\n---------------------------------------\r\n");
    // uart_send_string("Free Page\r\n");
    // uart_send_string("---------------------------------------\r\n");

    entry_t* start_frame = NULL;
    entry_t* last_frame = NULL;
    int alloc_status;

    for (int i = 0; i < MAX_ALLOC_CNT; i++) {
        if (alloc_frame_list[i]->head->ind == start_frame_ind) {
            start_frame = alloc_frame_list[i]->head;
            last_frame = alloc_frame_list[i]->tail;
            alloc_status = alloc_frame_list[i]->status;
            alloc_frame_list[i]->head = NULL;
            alloc_frame_list[i]->tail = NULL;
            alloc_frame_list[i]->status = -1;
            break;
        }
    }

    if (start_frame == NULL) {
        uart_send_string("Frame not allocated! Free page failed!\r\n");
        return;
    }

    start_frame->status = alloc_status;

    // uart_send_string("start_frame->ind: ");
    // uart_send_int(start_frame->ind);
    // uart_send_string("\r\n");

    // uart_send_string("alloc_status: ");
    // uart_send_int(alloc_status);
    // uart_send_string("\r\n");

    int buddy_ind = find_buddy(start_frame->ind, alloc_status);
    // uart_send_string("buddy_ind: ");
    // uart_send_int(buddy_ind);
    // uart_send_string("\r\n");

    // uart_send_string("alloc_status: ");
    // uart_send_int(frame_arr[buddy_ind]->status);
    // uart_send_string("\r\n");

    // If buddy exists, merge them and move on to the next level.
    // Keep merging until reaching maximum list size or no buddy found.
    while (frame_arr[buddy_ind]->status == alloc_status && alloc_status < FREE_LIST_SIZE - 1) {
        // uart_send_string("In while\r\n");
        // uart_send_string("alloc_status: ");
        // uart_send_int(alloc_status);
        // uart_send_string("\r\n");

        // Update free list.
        entry_t* buddy = frame_arr[buddy_ind];

        // Buddy is at the head of the list.
        if (buddy->prev == buddy) {
            // Only page in the list.
            if (buddy->next == NULL) {
                free_list[alloc_status]->head = NULL;
                free_list[alloc_status]->tail = NULL;
            } else {
                free_list[alloc_status]->head = buddy->next;
                free_list[alloc_status]->head->prev = free_list[alloc_status]->head;
            }
        } else {
            if (buddy->next == NULL) {
                free_list[alloc_status]->tail = free_list[alloc_status]->tail->prev;
                free_list[alloc_status]->tail->next = NULL;
            } else {
                buddy->next->prev = buddy->prev;
                buddy->prev->next = buddy->next;
            }
        }
        buddy->next = NULL;
        buddy->prev = NULL;
        start_frame->next = NULL;
        start_frame->prev = NULL;

        int merge_block_start = (start_frame->ind < buddy_ind) ? start_frame->ind : buddy_ind;
        start_frame = frame_arr[merge_block_start];

        // Update the frame status to free.
        // uart_send_string("merge_block_start: ");
        // uart_send_int(merge_block_start);
        // uart_send_string("\r\n");

        // uart_send_string("frame_arr[merge_block_start]->status: ");
        // uart_send_int(frame_arr[merge_block_start]->status);
        // uart_send_string("\r\n");

        frame_arr[merge_block_start]->status = alloc_status + 1;
        for (int i = frame_arr[merge_block_start]->ind + 1; i < frame_arr[merge_block_start]->ind + power(2, alloc_status + 1); i++) {
            frame_arr[i]->status = FREE_FRAME;
        }
        buddy_ind = find_buddy(frame_arr[merge_block_start]->ind, alloc_status + 1);

        // uart_send_string("buddy_ind: ");
        // uart_send_int(buddy_ind);
        // uart_send_string("\r\n");
        // uart_send_string("Buddy_status: ");
        // uart_send_int(frame_arr[buddy_ind]->status);
        // uart_send_string("\r\n");
        alloc_status++;
    }

    // Insert newly freed frames into free list.
    //alloc_status--;
    if (free_list[alloc_status]->tail == NULL) {
        free_list[alloc_status]->head = start_frame;
        free_list[alloc_status]->tail = start_frame;
        start_frame->prev = start_frame;
        start_frame->next = NULL;
    } else {
        free_list[alloc_status]->tail->next = start_frame;
        start_frame->prev = free_list[alloc_status]->tail;
        start_frame->next = NULL;
        free_list[alloc_status]->tail = start_frame;
    }

}

int find_buddy(int ind, int exp) {
    return ind ^ (1 << exp);
}

void memory_reserve(uint64_t start, uint64_t end) {
    int start_frame;
    int end_frame;

    uart_send_string("start frame: ");

    start_frame = (start - FREE_MEM_BASE_ADDR) / FRAME_SIZE;
    uart_send_int(start_frame);
    uart_send_string("\r\n");

    uart_send_string("end frame: ");
    end_frame = (end - FREE_MEM_BASE_ADDR) / FRAME_SIZE;
    uart_send_int(end_frame);
    uart_send_string("\r\n");

    for (int i = start_frame; i <= end_frame; i++) {
        frame_arr[i]->status = USED_FRAME;
    }
}

void show_free_list(void) {

    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Free Memory List\r\n");
    uart_send_string("---------------------------------------\r\n");

    for (int i = 0; i < FREE_LIST_SIZE; i++) {
        uart_send_string("Current list -> ");
        uart_send_int(i);
        uart_send_string(": ");
        entry_t* cur = free_list[i]->head;

        int cnt = 0;
        while (cur != NULL && cnt <= 10) {
            uart_send_int(cur->ind);
            uart_send_string(" ");
            cur = cur->next;
            cnt++;
        }
        uart_send_string("\r\n");
    }
    uart_send_int(free_list[FREE_LIST_SIZE - 1]->tail->prev->ind);
    uart_send_string(" ");
    uart_send_int(free_list[FREE_LIST_SIZE - 1]->tail->ind);
    uart_send_string(" ");
}

void show_alloc_list(void) {
    for (int i = 0; i < MAX_ALLOC_CNT; i++) {
        if (alloc_frame_list[i]->head != NULL) {
            int cur_frame = alloc_frame_list[i]->head->ind;
            int end_frame = alloc_frame_list[i]->tail->ind;
            
            for (; cur_frame <= end_frame; cur_frame++) {
                uart_send_int(frame_arr[cur_frame]->ind);
                uart_send_string(" ");
            }
            
            uart_send_string("\r\n");
        }
    }
}