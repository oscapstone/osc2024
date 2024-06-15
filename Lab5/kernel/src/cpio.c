#include "cpio.h"
#include "arm/sysregs.h"
#include "def.h"
#include "dtb.h"
#include "entry.h"
#include "fork.h"
#include "memory.h"
#include "mini_uart.h"
#include "slab.h"
#include "string.h"
#include "thread.h"

/* CPIO archive format
 * https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
 */

/* CPIO new ASCII format
 * The  "new"  ASCII format uses 8-byte hexadecimal fields for all numbers
 */
struct __attribute__((packed)) cpio_newc_header {
    char c_magic[6];  // "070701"
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};

enum cpio_mode { CPIO_LIST, CPIO_CAT, CPIO_EXEC, CPIO_LOAD };

static inline void cpio_print_filename(char* name_addr, unsigned int name_size)
{
    for (unsigned int i = 0; i < name_size; i++)
        uart_send(name_addr[i]);
    uart_send_string("\n");
}

static inline void cpio_print_content(char* file_addr, unsigned int file_size)
{
    for (unsigned int i = 0; i < file_size; i++) {
        if (file_addr[i] == '\n')
            uart_send('\r');
        uart_send(file_addr[i]);
    }
}

static inline void cpio_exec_program(char* file_addr, unsigned int file_size)
{
    void* target = kmalloc(file_size, 0);
    memcpy(target, file_addr, file_size);
    copy_process(PF_KTHREAD, kernel_process, target);

    // if (!current_task->user_stack)
    //     current_task->user_stack = kmalloc(THREAD_STACK_SIZE, 0);

    // unsigned long spsr_el1 =
    //     (SPSR_MASK_D | SPSR_MASK_A | SPSR_MASK_F | SPSR_EL0t);

    // asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1));
    // asm volatile("msr elr_el1, %0" : : "r"(target));
    // asm volatile("msr sp_el0, %0"
    //              :
    //              : "r"(current_task->user_stack + THREAD_STACK_SIZE));
    // asm volatile("eret");
}

static inline void cpio_load_program(char* file_addr, unsigned int file_size)
{
    void* target = kmalloc(file_size, 0);
    memcpy(target, file_addr, file_size);
    move_to_user_mode((unsigned long)target);
}


/*
 * CPIO archive will be stored like this:
 *
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |       (file name)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |       (file data)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |            .             |
 * |            .             |
 * |            .             |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |        TRAILER!!!        |
 * +--------------------------+
 */

static uintptr_t _cpio_start;
static uintptr_t _cpio_end;

uintptr_t get_cpio_start(void)
{
    return _cpio_start;
}

void set_cpio_start(uintptr_t ptr)
{
    _cpio_start = ptr;
}

uintptr_t get_cpio_end(void)
{
    return _cpio_end;
}

void set_cpio_end(uintptr_t ptr)
{
    _cpio_end = ptr;
}

int cpio_init(void)
{
    return fdt_traverse(fdt_find_cpio_ptr);
}

static int cpio_parse(enum cpio_mode mode, char* file_name)
{
    char* cpio = (char*)_cpio_start;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio;

    while (!str_n_cmp(header->c_magic, CPIO_MAGIC, str_len(CPIO_MAGIC))) {
        char* file = (char*)header + sizeof(struct cpio_newc_header);

        if (!str_cmp(file, CPIO_MAGIC_FOOTER))
            return mode == CPIO_LIST ? CPIO_EXIT_SUCCESS : CPIO_EXIT_ERROR;

        unsigned int name_size = hexstr2int(header->c_namesize);
        unsigned int file_size = hexstr2int(header->c_filesize);
        char* file_content = (char*)mem_align(file + name_size, 4);

        switch (mode) {
        case CPIO_LIST:
            cpio_print_filename(file, name_size);
            break;

        case CPIO_CAT:
            if (!str_cmp(file, file_name)) {
                cpio_print_content(file_content, file_size);
                return CPIO_EXIT_SUCCESS;
            }
            break;

        case CPIO_EXEC:
            if (!str_cmp(file, file_name)) {
                cpio_exec_program(file_content, file_size);
                return CPIO_EXIT_SUCCESS;
            }
            break;
        case CPIO_LOAD:
            if (!str_cmp(file, file_name)) {
                cpio_load_program(file_content, file_size);
                return CPIO_EXIT_SUCCESS;
            }
            break;
        default:
            break;
        }

        header =
            (struct cpio_newc_header*)mem_align(file_content + file_size, 4);
    }

    return CPIO_EXIT_ERROR;
}

void cpio_ls(void)
{
    int status = cpio_parse(CPIO_LIST, NULL);
    if (status == CPIO_EXIT_ERROR)
        uart_send_string("Parse Error\n");
}

void cpio_cat(char* file_name)
{
    int status = cpio_parse(CPIO_CAT, file_name);
    if (status == CPIO_EXIT_ERROR) {
        uart_send_string("File '");
        uart_send_string(file_name);
        uart_send_string("' not found\n");
    }
}

void cpio_exec(char* file_name)
{
    int status = cpio_parse(CPIO_EXEC, file_name);
    if (status == CPIO_EXIT_ERROR) {
        uart_send_string("Program '");
        uart_send_string(file_name);
        uart_send_string("' not found\n");
    }
}

int cpio_load(char* file_name)
{
    int status = cpio_parse(CPIO_LOAD, file_name);
    if (status == CPIO_EXIT_ERROR) {
        uart_send_string("Program '");
        uart_send_string(file_name);
        uart_send_string("' not found\n");
        return -1;
    }
    return 0;
}
