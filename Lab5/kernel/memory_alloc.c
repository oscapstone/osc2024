#include "memory_alloc.h"
#include "alloc.h"
#include "mini_uart.h"

// mpool[i] stores the corresponding information for frame[i].
mem_pool* mpool;

void init_memory_pool(void) {

    mpool = (mem_pool *)malloc(sizeof(mem_pool) * (free_mem_size / BLOCK_SIZE));

    // for (uint64_t i = 0; i < 10; i++) {
    //     mpool[i].allocate_all = 0;
    //     mpool[i]._32B_available = 8;
    //     mpool[i]._64B_available = 4;
    //     mpool[i]._128B_available = 4;
    //     mpool[i]._256B_available = 4;
    //     mpool[i]._512B_available = 2;
    //     mpool[i]._1024B_available = 1;

    //     for (int j = 0; j < 8; j++) {
    //         mpool[i]._32B[j].free = 1;

    //         if (j < 1) {
    //             mpool[i]._1024B[j].free = 1;
    //         } 
            
    //         if (j < 2) {
    //             mpool[i]._512B[j].free = 1;
    //         }
            
    //         // 64, 128, 256 bytes only have 4 slots.
    //         if (j < 4) {
    //             mpool[i]._64B[j].free = 1;
    //             mpool[i]._128B[j].free = 1;
    //             mpool[i]._256B[j].free = 1;
    //         }
    //     }
    // }
}


uint64_t dynamic_malloc(uint64_t request_size) {

    uart_send_string("\r\n---------------------------------------\r\n");
    uart_send_string("Dynamic Allocator\r\n");
    uart_send_string("---------------------------------------\r\n");

    
    // If the request size is larger than 1KB, allocate entire frame(or several frames) for the request.
    if (request_size > 1024) {
        allocate_info* info = allocate_frame(request_size);

        if (info == NULL) {
            uart_send_string("Request exceeded largest frame size!\r\n");
            return -1;
        }

        uart_send_string("Allocated frames: ");

        for (int i = info->start_frame; i <= info->last_frame; i++) {
            uart_send_int(i);
            uart_send_string(" ");
            mpool[i].allocate_all = 1;
        }

        uart_send_string("for request ");
        uart_send_int(request_size);
        uart_send_string(" bytes\r\n");

        return info->start_frame * 4096 + FREE_MEM_BASE_ADDR;
    } else {
        // Check allocated memory for remaining slots available.
        for (int i = 0; i < MAX_ALLOC; i++) {
            if (alloc_mem_list[i] != NULL) {
                for (int j = alloc_mem_list[i]->start_frame; j <= alloc_mem_list[i]->last_frame; j++) {
                    // The entire frame(or several frames) is allocated for a single request.
                    if (mpool[j].allocate_all) {
                        break;
                    }

                    if (request_size <= 32) {
                        // Current frame has 32 byte slot left.
                        if (mpool[j]._32B_available > 0) {
                            mpool[j]._32B_available--;
                            for (int s = 0; s < 8; s++) {
                                if (mpool[j]._32B[s].free) {
                                    mpool[j]._32B[s].free = 0;

                                    uart_send_string("Allocated slot ");
                                    uart_send_int(s);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                    return (j * 4096 + FREE_MEM_BASE_ADDR) + 32 * s;
                                }
                            }
                        } else {
                            continue;
                        }
                    } else if (request_size > 32 && request_size <= 64) {
                        if (mpool[j]._64B_available > 0) {
                            mpool[j]._64B_available--;
                            for (int s = 0; s < 4; s++) {
                                if (mpool[j]._64B[s].free) {
                                    mpool[j]._64B[s].free = 0;

                                    uart_send_string("Allocated slot ");
                                    uart_send_int(s + 8);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                    return (j * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * s);
                                }
                            }
                        } else {
                            continue;
                        }
                    } else if (request_size > 64 && request_size <= 128) {
                        if (mpool[j]._128B_available > 0) {
                            mpool[j]._128B_available--;
                            for (int s = 0; s < 4; s++) {
                                if (mpool[j]._128B[s].free) {
                                    mpool[j]._128B[s].free = 0;

                                    uart_send_string("Allocated slot ");
                                    uart_send_int(s + 12);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                    return (j * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * s);
                                }
                            }
                        } else {
                            continue;
                        }
                    } else if (request_size > 128 && request_size <= 256) {
                        if (mpool[j]._256B_available > 0) {
                            mpool[j]._256B_available--;
                            for (int s = 0; s < 4; s++) {
                                if (mpool[j]._256B[s].free) {
                                    mpool[j]._256B[s].free = 0;

                                    uart_send_string("Allocated slot ");
                                    uart_send_int(s + 16);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                    return (j * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4) + (256 * s);
                                }
                            }
                        } else {
                            continue;
                        }
                    } else if (request_size > 256 && request_size <= 512) {
                        if (mpool[j]._512B_available > 0) {
                            mpool[j]._512B_available--;
                            for (int s = 0; s < 2; s++) {
                                if (mpool[j]._512B[s].free) {
                                    mpool[j]._512B[s].free = 0;

                                    uart_send_string("Allocated slot ");
                                    uart_send_int(s + 20);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                    return (j * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4) + (256 * 4) + (512 * s);
                                }
                            }
                        } else {
                            continue;
                        }
                    } else if (request_size > 512 && request_size <= 1024) {
                        if (mpool[j]._1024B_available > 0) {
                            mpool[j]._1024B_available--;

                            if (mpool[j]._1024B[0].free) {
                                mpool[j]._1024B[0].free = 0;

                                uart_send_string("Allocated slot ");
                                    uart_send_int(22);
                                    uart_send_string(" in frame ");
                                    uart_send_int(j);
                                    uart_send_string(" for request ");
                                    uart_send_int(request_size);
                                    uart_send_string(" bytes\r\n");

                                return (j * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4) + (256 * 4) + (512 * 2);
                            }
                            
                        } else {
                            continue;
                        }
                    }
                }
            }
        }

        // No remaining slots available. Allocate new frame.
        allocate_info* info = allocate_frame(request_size);

        // Initialize memory pool for new frame.
        mpool[info->start_frame].allocate_all = 0;
        mpool[info->start_frame]._32B_available = 8;
        mpool[info->start_frame]._64B_available = 4;
        mpool[info->start_frame]._128B_available = 4;
        mpool[info->start_frame]._256B_available = 4;
        mpool[info->start_frame]._512B_available = 2;
        mpool[info->start_frame]._1024B_available = 1;

        for (int j = 0; j < 8; j++) {
            mpool[info->start_frame]._32B[j].free = 1;

            if (j < 1) {
                mpool[info->start_frame]._1024B[j].free = 1;
            } 
            
            if (j < 2) {
                mpool[info->start_frame]._512B[j].free = 1;
            }
            
            // 64, 128, 256 bytes only have 4 slots.
            if (j < 4) {
                mpool[info->start_frame]._64B[j].free = 1;
                mpool[info->start_frame]._128B[j].free = 1;
                mpool[info->start_frame]._256B[j].free = 1;
            }
        }

        if (request_size <= 32) {
            mpool[info->start_frame]._32B_available--;
            mpool[info->start_frame]._32B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(0);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");

            return info->start_frame * 4096 + FREE_MEM_BASE_ADDR;
        } else if (request_size > 32 && request_size <= 64) {
            mpool[info->start_frame]._64B_available--;
            mpool[info->start_frame]._64B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(8);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");
            
            return (info->start_frame * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8);
        } else if (request_size > 64 && request_size <= 128) {
            mpool[info->start_frame]._128B_available--;
            mpool[info->start_frame]._128B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(12);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");
            
            return (info->start_frame * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4);
        } else if (request_size > 128 && request_size <= 256) {
            mpool[info->start_frame]._256B_available--;
            mpool[info->start_frame]._256B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(16);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");
            
            return (info->start_frame * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4);
        } else if (request_size > 256 && request_size <= 512) {
            mpool[info->start_frame]._512B_available--;
            mpool[info->start_frame]._512B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(20);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");
            
            return (info->start_frame * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4) + (256 * 4);
        } else if (request_size > 512 && request_size <= 1024) {
            mpool[info->start_frame]._1024B_available--;
            mpool[info->start_frame]._1024B[0].free = 0;

            uart_send_string("Allocated slot ");
            uart_send_int(22);
            uart_send_string(" in frame ");
            uart_send_int(info->start_frame);
            uart_send_string(" for request ");
            uart_send_int(request_size);
            uart_send_string(" bytes\r\n");
            
            return (info->start_frame * 4096 + FREE_MEM_BASE_ADDR) + (32 * 8) + (64 * 4) + (128 * 4) + (256 * 4) + (512 * 2);
        }
    }
}