#include "video.h"
#include "cpio.h"
#include "fork.h"
#include "string.h"
#include "io.h"
#include "alloc.h"
#include "lib.h"

extern struct task_struct *current;
extern void memzero_asm(unsigned long src, unsigned long n);
#ifndef QEMU
extern unsigned long CPIO_START_ADDR_FROM_DT;
#endif

void start_video(int argc, char *argv[])
{

#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif

    uint32_t head_size = sizeof(cpio_newc_header);

    char *target_name = "syscall.img";

    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            printf("\nFile not found");
            break;
        }
        else if(strcmp(filename, target_name) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;

            printf("\r\n[INFO] File found: "); printf(target_name); printf(" in CPIO at "); printf_hex((unsigned long)filedata);
            printf("\r\n[INFO] File Size: "); printf_hex(filesize);

            void* user_program_addr = balloc(filesize+THREAD_SIZE); // extra page in case
            if(user_program_addr == NULL) return;
            memzero_asm((unsigned long)user_program_addr, filesize+THREAD_SIZE);
            for(int i=0; i<filesize; i++){
                ((char*)user_program_addr)[i] = filedata[i];
            }

            preempt_disable(); // leads to get the correct current task

            current->state = TASK_STOPPED;

            unsigned long tmp;
            asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
            tmp |= 1;
            asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

            copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)user_program_addr, 0);

            preempt_enable();
            break;
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }
    return;
}