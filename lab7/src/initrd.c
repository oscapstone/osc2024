#include <stdint.h>

#include "initrd.h"
#include "irq.h"
#include "mm.h"
#include "proc.h"
#include "string.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"

void *RAMFS_BASE = (void *)0x8000000;

// Convert hexadecimal string to int
// @param s: hexadecimal string
// @param n: string length
int hextoi(char *s, int n)
{
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= 0)
            r += *s++ - '0';
    }
    return r;
}

// TODO: Extract initrd_list() and pass in function pointers

void initrd_list()
{
    char *fptr = (char *)RAMFS_BASE;

    // Check if the file is encoded with New ASCII Format
    while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
        cpio_t *header = (cpio_t *)fptr;

        // New ASCII Format uses 8-byte hexadecimal string for all numbers
        int namesize = hextoi(header->c_namesize, 8);
        int filesize = hextoi(header->c_filesize, 8);

        // Total size of (header + pathname) is a multiple of four bytes
        // File data is also padded to a multiple of four bytes
        int headsize = align4(sizeof(cpio_t) + namesize);
        int datasize = align4(filesize);

        // Print file pathname
        char pathname[namesize];
        strncpy(pathname, fptr + sizeof(cpio_t), namesize);
        uart_puts(pathname);
        uart_putc('\n');

        fptr += headsize + datasize;
    }
}

void initrd_cat(const char *target)
{
    char *fptr = (char *)RAMFS_BASE;
    while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
        cpio_t *header = (cpio_t *)fptr;
        int namesize = hextoi(header->c_namesize, 8);
        int filesize = hextoi(header->c_filesize, 8);
        int headsize = align4(sizeof(cpio_t) + namesize);
        int datasize = align4(filesize);
        char pathname[namesize];
        strncpy(pathname, fptr + sizeof(cpio_t), namesize);
        if (!strcmp(target, pathname)) {
            char data[filesize + 1];
            memset(data, 0, filesize + 1);
            strncpy(data, fptr + headsize, filesize);
            uart_puts(data);
            uart_putc('\n');
            return;
        }
        fptr += headsize + datasize;
    }
    uart_puts("File not found.\n");
}

void initrd_exec(const char *target)
{
    char *fptr = (char *)RAMFS_BASE;
    while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
        cpio_t *header = (cpio_t *)fptr;
        int namesize = hextoi(header->c_namesize, 8);
        int filesize = hextoi(header->c_filesize, 8);
        int headsize = align4(sizeof(cpio_t) + namesize);
        int datasize = align4(filesize);
        char pathname[namesize];
        strncpy(pathname, fptr + sizeof(cpio_t), namesize);
        if (!strcmp(target, pathname)) {
            disable_interrupt();

            void *program = kmalloc(filesize);
            memcpy(program, fptr + headsize, filesize);

            struct task_struct *task = kthread_create(program);
            task->code_size = filesize;

            map_pages((unsigned long)task->pgd, 0x0, filesize,
                      (unsigned long)VIRT_TO_PHYS(program), 0);
            map_pages((unsigned long)task->pgd, 0xFFFFFFFFB000, 0x4000,
                      (unsigned long)VIRT_TO_PHYS(task->user_stack), 0);
            map_pages((unsigned long)task->pgd, 0x3C000000, 0x3000000,
                      0x3C000000, 0);

            vfs_open("/dev/uart", 0, &task->fdt[0]); // stdin
            vfs_open("/dev/uart", 0, &task->fdt[1]); // stdout
            vfs_open("/dev/uart", 0, &task->fdt[2]); // stderr

            asm volatile("dsb ish");
            asm volatile("msr ttbr0_el1, %0" ::"r"(VIRT_TO_PHYS(task->pgd)));
            asm volatile("tlbi vmalle1is");
            asm volatile("dsb ish");
            asm volatile("isb");

            asm volatile("msr tpidr_el1, %0;" ::"r"(task));
            asm volatile("msr spsr_el1, %0" ::"r"(0x340));
            asm volatile("msr elr_el1, %0" ::"r"(0x0));
            asm volatile("msr sp_el0, %0" ::"r"(0xFFFFFFFFF000));
            asm volatile("mov sp, %0" ::"r"(task->stack + STACK_SIZE));
            asm volatile("eret");
        }
        fptr += headsize + datasize;
    }
    uart_puts("File not found.\n");
}

void initrd_callback(void *addr)
{
    uart_puts("[INFO] Initrd is mounted at ");
    uart_hex((uintptr_t)addr);
    uart_putc('\n');
    RAMFS_BASE = (char *)PHYS_TO_VIRT(addr);
}
