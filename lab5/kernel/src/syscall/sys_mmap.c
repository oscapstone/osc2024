

#include "syscall.h"
#include "arm/mmu.h"
#include "lib/mmap.h"
#include "proc/task.h"
#include "mm/mm.h"
#include "utils/printf.h"

UPTR mmap_find_region_in_heap(U32 page_count) {
    TASK* task = task_get_current_el1();

    BOOL found_region = FALSE;
    UPTR heap_offset = MMU_HEAP_BASE;       // 從這裡開始找
    while (!found_region) {
        found_region = TRUE;
        for (U32 i = 0; i < page_count; i++) {
            for(U32 j = 0; j < task->mm.user_pages_count; j++) {
                USER_PAGE_INFO* page_info = &task->mm.user_pages[j];
                if (heap_offset + i * PD_PAGE_SIZE == page_info->v_addr) {
                    found_region = FALSE;
                    break;
                }
            }
            if (!found_region) {
                break;
            }
        }
        if (!found_region) {
            heap_offset += PD_PAGE_SIZE;
        }
    }
    return heap_offset;
}

void sys_mmap(TRAP_FRAME* trap_frame) {
    UPTR v_addr = trap_frame->regs[0];
    U64 len = trap_frame->regs[1];
    int prot = (int) trap_frame->regs[2];
    U32 flags = (U32) trap_frame->regs[3];
    U32 file_offset = (U32) trap_frame->regs[4];    // don't know what it can do

    TASK* task = task_get_current_el1();

    if (len == 0) {
        // error len == 0
        trap_frame->regs[0] == 0;
        return;
    }

    U32 page_count = (len + PD_PAGE_SIZE - 1) / PD_PAGE_SIZE;

    NS_DPRINT("[SYSCALL][MMAP] try mapping. v_addr = 0x%08x%08x, page_count = %d\n", v_addr >> 32, v_addr, page_count);

    if (v_addr == NULL) {
        v_addr = mmap_find_region_in_heap(page_count);
    } else {
        v_addr = v_addr % 0x1000 ? v_addr + (0x1000 - v_addr % 0x1000) : v_addr;
        
        BOOL is_overlap = FALSE;
        for (U32 i = 0; i < page_count; i++) {
            for(U32 j = 0; j < task->mm.user_pages_count; j++) {
                USER_PAGE_INFO* page_info = &task->mm.user_pages[j];
                if (v_addr + i * PD_PAGE_SIZE == page_info->v_addr) {
                    is_overlap = TRUE;
                    break;
                }
            }
            if (is_overlap) {
                break;
            }
        }
        if (is_overlap) {
            v_addr = mmap_find_region_in_heap(page_count);
        }
    }

    U32 page_flags = 0;
    if (prot & PROT_READ) {
        page_flags |= TASK_USER_PAGE_INFO_FLAGS_READ;
    }
    if (prot & PROT_WRITE) {
        page_flags |= TASK_USER_PAGE_INFO_FLAGS_WRITE;
    }
    if (prot & PROT_EXEC) {
        page_flags |= TASK_USER_PAGE_INFO_FLAGS_EXEC;
    }
    if (flags & MAP_ANONYMOUS) {
        page_flags |= TASK_USER_PAGE_INFO_FLAGS_ANONYMOUS;
    }
    if (flags & MAP_POPULATE) {
        for (U32 i = 0; i < page_count; i++) {
            UPTR new_page;
            if (flags & MAP_ANONYMOUS) {
                new_page = kzalloc(PD_PAGE_SIZE);
            } else {
                new_page = kmalloc(PD_PAGE_SIZE);
            }
            mmu_map_page(task, v_addr + i * PD_PAGE_SIZE, new_page, page_flags);
        }
    } else {
        // because we already know the page in not in user page now
        for (U32 i = 0; i < page_count; i++) {
            USER_PAGE_INFO* user_page = &task->mm.user_pages[task->mm.user_pages_count++];
            user_page->flags = page_flags;
            user_page->v_addr = v_addr + i * PD_PAGE_SIZE;
            user_page->p_addr = 0;              // implement blank page
            NS_DPRINT("[SYSCALL][MMAP] Adding new blank virtual region. v_addr: 0x%08x%08x\n", user_page->v_addr >> 32, user_page->v_addr);
        }
    }

    NS_DPRINT("[SYSCALL][MMAP] map successfully. v_addr = 0x%08x%08x\n", v_addr >> 32, v_addr);
    trap_frame->regs[0] = v_addr;
    return;
}
