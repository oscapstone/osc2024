#include "dynamic_alloc.h"
#include "alloc.h"
#include "mini_uart.h"

slab_cache_t* slab_cache;

// Total frames from 0x0 to 0x3C000000 -> 245697 frames
void init_dynamic_alloc(void) {
    init_buddy_system();

    // The cache contains lists of slabs of size 64B, 128B, 256B.
    slab_cache = (slab_cache_t *)malloc(sizeof(slab_cache_t) * 3);
    for (int i = 0; i < 3; i++) {
        slab_cache[i].head = NULL;
        slab_cache[i].tail = NULL;
    }
}

uint64_t dynamic_alloc(uint64_t request) {
    // Request larger than 256 bytes, allocate all frames.
    if (request > 256) {
        // Get the index of alloc_frame_list.
        int alloc_ind = alloc_page(request);

        // Uncomment for demo.
        // uart_send_string("Allocated frames ");
        for (int i = alloc_frame_list[alloc_ind]->head->ind; i <= alloc_frame_list[alloc_ind]->tail->ind; i++) {
            // For contiguous allocated frames, the slot_size records the offset between the current frame 
            // and the head.
            frame_arr[i]->slot_size = i - alloc_frame_list[alloc_ind]->head->ind;
            frame_arr[i]->slot_cnt = 0;

            // Uncomment for demo.
            // uart_send_int(i);
            // uart_send_string(" ");
        }

        // Uncomment for demo.
        // uart_send_string("\r\n");

        return alloc_frame_list[alloc_ind]->head->ind * FRAME_SIZE + FREE_MEM_BASE_ADDR;
    } else {
        // For determining which slot the request belong to.
        int slab_cache_ind;
        int slot_cnt;
        int slot_size;
        if (request > 128) {
            slab_cache_ind = 2;
            slot_cnt = 16;
            slot_size = 256;
        } else if (request > 64) {
            slab_cache_ind = 1;
            slot_cnt = 32;
            slot_size = 128;
        } else {
            slab_cache_ind = 0;
            slot_cnt = 64;
            slot_size = 64;
        }

        // Allocated slabs exist.
        if (slab_cache[slab_cache_ind].head != NULL) {
            entry_t* cur_entry = slab_cache[slab_cache_ind].head;

            // Find a slab with empty slots.
            while (cur_entry != NULL && cur_entry->slot_cnt <= 0)
                cur_entry = cur_entry->next_slot;

            // Find empty slot.
            if (cur_entry != NULL) {
                short free_slot_ind = 0;
                
                for (; free_slot_ind < slot_cnt; free_slot_ind++) {
                    if (cur_entry->bitmap & (1 << free_slot_ind)) {
                        // Mark the slot as used.
                        cur_entry->bitmap &= ~(1 << free_slot_ind);
                        cur_entry->slot_cnt--;
                        break;
                    }
                }
                
                // Uncomment for demo.
                // uart_send_string("Allocated slot ");
                // uart_send_int(free_slot_ind);
                // uart_send_string(" in frame ");
                // uart_send_int(cur_entry->ind);
                // uart_send_string("\r\n");

                return cur_entry->ind * FRAME_SIZE + FREE_MEM_BASE_ADDR + free_slot_ind * slot_size;
            } else {
                // Allocate a new page if no slot remains.
                int alloc_ind = alloc_page(request);
                cur_entry = alloc_frame_list[alloc_ind]->head;

                if (slot_size == 256) {
                    cur_entry->bitmap = 0xFFFF;
                    cur_entry->slot_cnt = 16;
                    cur_entry->slot_size = SLOT_256B;
                } else if (slot_size == 128) {
                    cur_entry->bitmap = 0xFFFFFFFF;
                    cur_entry->slot_cnt = 32;
                    cur_entry->slot_size = SLOT_128B;
                } else {
                    cur_entry->bitmap = 0xFFFFFFFFFFFFFFFF;
                    cur_entry->slot_cnt = 64;
                    cur_entry->slot_size = SLOT_64B;
                }

                cur_entry->bitmap &= ~(0x1);
                cur_entry->slot_cnt--;

                // Insert into slab cache.
                slab_cache[slab_cache_ind].tail->next = cur_entry;
                cur_entry->prev_slot = slab_cache[slab_cache_ind].tail;
                cur_entry->next_slot = NULL;
                slab_cache[slab_cache_ind].tail = cur_entry;

                // Uncomment for demo.
                // uart_send_string("Allocated slot ");
                // uart_send_int(0);
                // uart_send_string(" in frame ");
                // uart_send_int(cur_entry->ind);
                // uart_send_string("\r\n");

                return cur_entry->ind * FRAME_SIZE + FREE_MEM_BASE_ADDR;
            }
        // No allocated page exists in the cache.
        } else {
            int alloc_ind = alloc_page(request);
            entry_t* cur_entry = alloc_frame_list[alloc_ind]->head;
            cur_entry->prev_slot = cur_entry;
            cur_entry->next_slot = NULL;

            slab_cache[slab_cache_ind].head = cur_entry;
            slab_cache[slab_cache_ind].tail = cur_entry;

            if (slot_size == 256) {
                cur_entry->bitmap = 0xFFFF;
                cur_entry->slot_cnt = 16;
                cur_entry->slot_size = SLOT_256B;
            } else if (slot_size == 128) {
                cur_entry->bitmap = 0xFFFFFFFF;
                cur_entry->slot_cnt = 32;
                cur_entry->slot_size = SLOT_128B;
            } else {
                cur_entry->bitmap = 0xFFFFFFFFFFFFFFFF;
                cur_entry->slot_cnt = 64;
                cur_entry->slot_size = SLOT_64B;
            }

            cur_entry->bitmap &= ~(0x1);
            cur_entry->slot_cnt--;

            // Uncomment for demo.
            // uart_send_string("Allocated slot ");
            // uart_send_int(0);
            // uart_send_string(" in frame ");
            // uart_send_int(cur_entry->ind);
            // uart_send_string("\r\n");

            return cur_entry->ind * FRAME_SIZE + FREE_MEM_BASE_ADDR;
        }
    }
}

void dynamic_free(uint64_t addr) {
    // Find the frame it belongs to.
    int frame_ind = (addr - FREE_MEM_BASE_ADDR) / FRAME_SIZE;

    if (frame_arr[frame_ind]->slot_size == SLOT_64B || frame_arr[frame_ind]->slot_size == SLOT_128B || frame_arr[frame_ind]->slot_size == SLOT_256B) {
        int slot_size;
        int slab_cache_ind;

        if (frame_arr[frame_ind]->slot_size == SLOT_64B) {
            slab_cache_ind = 0;
            slot_size = 64;
        } else if (frame_arr[frame_ind]->slot_size == SLOT_128B) {
            slab_cache_ind = 1;
            slot_size = 128;
        } else if (frame_arr[frame_ind]->slot_size == SLOT_256B) {
            slab_cache_ind = 2;
            slot_size = 256;
        }

        int slot_ind = (addr - (frame_ind * FRAME_SIZE + FREE_MEM_BASE_ADDR)) / slot_size;
        // The slot is free already.
        if (((1 << slot_ind) & frame_arr[frame_ind]->bitmap) == (1 << slot_ind)) {
            uart_send_string("Slot ");
            uart_send_int(slot_ind);
            uart_send_string(" in frame ");
            uart_send_int(frame_ind);
            uart_send_string(" is freed already!\r\n");
            return;
        }

        frame_arr[frame_ind]->bitmap |= (1 << slot_ind);
        frame_arr[frame_ind]->slot_cnt++;

        // Uncomment for demo.
        // uart_send_string("Freeing slot ");
        // uart_send_int(slot_ind);
        // uart_send_string(" in frame ");
        // uart_send_int(frame_ind);
        // uart_send_string("\r\n");
        // uart_send_string("Remaining slots: ");
        // uart_send_int(frame_arr[frame_ind]->slot_cnt);
        // uart_send_string("\r\n");
        
        // Free the page if all allocated slots are freed.
        if (frame_arr[frame_ind]->slot_cnt == slot_size) {
            frame_arr[frame_ind]->slot_size = SLOT_NOT_ALLOCATED;
            free_page(frame_ind);

            // Discard the frame in slab cache.
            entry_t* cur_entry = slab_cache[slab_cache_ind].head;

            while (cur_entry != NULL && cur_entry->ind != frame_ind) {
                cur_entry = cur_entry->next_slot;
            }

            // If current frame is at the head of cache.
            if (cur_entry->prev_slot == cur_entry) {
                slab_cache[slab_cache_ind].head = cur_entry->next_slot;
                if (slab_cache[slab_cache_ind].head == NULL) {
                    slab_cache[slab_cache_ind].tail = NULL;
                } else {
                    slab_cache[slab_cache_ind].head->prev_slot = slab_cache[slab_cache_ind].head;
                }
            // At the end or middle of cache.
            } else {
                if (cur_entry->next_slot == NULL) {
                    slab_cache[slab_cache_ind].tail = cur_entry->prev_slot;
                    slab_cache[slab_cache_ind].tail->next = NULL;
                } else {
                    cur_entry->prev_slot->next_slot = cur_entry->next_slot;
                    cur_entry->next_slot->prev_slot = cur_entry->prev_slot;
                }
            }
            cur_entry->next_slot = NULL;
            cur_entry->prev_slot = NULL;
        }
    } else if (frame_arr[frame_ind]->slot_size == SLOT_NOT_ALLOCATED) {
        uart_send_string("Memory 0x");
        uart_send_int(addr);
        uart_send_string(" not allocated!\r\n");
    // Contiguous frames to free.
    } else {
        int head = frame_ind - frame_arr[frame_ind]->slot_size;
        free_page(head);
    }
}