#include "initrd.h"
#include "type.h"
#include "uart.h"
#include "memory.h"
#include "string.h"
#include "cpio.h"
#include "sched.h"
#include "task.h"
#include "kernel.h"
#include "thread.h"


typedef struct {

    byteptr_t name;
    byteptr_t content;
    uint32_t size;

} finfo_t;

typedef finfo_t* finfoptr_t;


static byteptr_t
_initrd_next(const byteptr_t cpio_addr, finfoptr_t info)
{
    byteptr_t _addr = (byteptr_t) cpio_addr;

    if (!memory_cmp(_addr, "070701", 6) && memory_cmp(_addr + sizeof(cpio_t), "TRAILER!!!", 10)) {

        cpio_ptr_t header = (cpio_ptr_t) _addr;

        uint64_t name_size = ascii_to_uint64(header->c_namesize, (uint32_t) sizeof(header->c_namesize));
        uint64_t file_size = ascii_to_uint64(header->c_filesize, (uint32_t) sizeof(header->c_filesize));

        uint64_t content_offset  = (uint64_t) memory_padding((const byteptr_t) (sizeof(cpio_t) + name_size), 2);
        uint64_t total_size      = (uint64_t) memory_padding((const byteptr_t) (content_offset + file_size), 2);

        info->name = _addr + sizeof(cpio_t);
        info->size = file_size;
        info->content = _addr + content_offset;

        return _addr + total_size;
    }

    return nullptr;
}


static byteptr_t
_initrd_find(const byteptr_t cpio_addr, const byteptr_t name, finfoptr_t info_ptr)
{
    byteptr_t curr = (byteptr_t) cpio_addr;
    byteptr_t next = _initrd_next(cpio_addr, info_ptr);

    while (next) {
        if (str_eql(info_ptr->name, name)) {
            return curr;
        }
        curr = next;
        next = _initrd_next(curr, info_ptr);
    }
    return nullptr;
}


static byteptr_t    _cpio_ptr = nullptr;
static byteptr_t    _cpio_end = nullptr;

byteptr_t           initrd_get_ptr() { return _cpio_ptr; }
void                initrd_set_ptr(byteptr_t ptr) { _cpio_ptr = ptr; }
byteptr_t           initrd_get_end() { return _cpio_end; }
void                initrd_set_end(byteptr_t ptr) { _cpio_end = ptr; }


static byteptr_t
_initrd_find_file(const byteptr_t name, finfoptr_t info_ptr)
{
    byteptr_t cpio_addr = initrd_get_ptr();
    byteptr_t file = _initrd_find(cpio_addr, name, info_ptr);
    if (!file) { uart_line("No such file or directory."); return 0; }
    if (info_ptr->size == 0) {  uart_line("It's a directory."); return 0; }
    return file;
}


void 
initrd_list() 
{
    byteptr_t cpio_addr = initrd_get_ptr(); 
    finfo_t info;
    byteptr_t next_addr = _initrd_next(cpio_addr, &info);
    while (next_addr) {
        mini_uart_putln(info.name);
        next_addr = _initrd_next(next_addr, &info);
    }
}


static void 
_print_file(const byteptr_t content, uint32_t size)
{
    byteptr_t cur = (byteptr_t) content;
    while (size--) {
        if (*cur == '\n') {
            uart_put('\r');
        }
        uart_put(*cur);
        cur++;
    }
    uart_endl();
}


void 
initrd_cat(const byteptr_t name)
{
    finfo_t info;
    byteptr_t file = _initrd_find_file(name, &info);
    if (file) { _print_file(info.content, info.size); }
}


static void 
run_user_process(uint64_t program) // a kernel thread of 
{
    uart_printf("[DEBUG] Kernel process (run_user_process) started. EL %d\r\n", get_el());
    int err = task_to_user_mode(current_task(), program);
	if (err < 0) {
		uart_printf("[DEBUG] Error while moving process to user mode\n");
	}
}


void
initrd_exec(const byteptr_t name)
{
    finfo_t info;
    byteptr_t file = _initrd_find_file(name, &info);
    if (file) { 
        uart_printf("[DEBUG] initrd_exec - info: %s, size: 0x%x\n", info.name, info.size);
        thread_create_user((uint64_t) run_user_process, (uint64_t) info.content, info.size);
    }
}



// #define USTACK_SIZE 0x1000

// static void
// _execute_user_program(byteptr_t content, byteptr_t ustack_sp, byteptr_t sp)
// {
//     asm volatile("mov   x0,       0x340  \n");                  // b'1101 00 0000
//     asm volatile("msr   spsr_el1, x0     \n");                  // unmask EL1 IRQ
//     asm volatile("msr   elr_el1,  %0     \n" ::"r"(content));
//     asm volatile("msr   sp_el0,   %0     \n" ::"r"(ustack_sp + USTACK_SIZE));
//     if (sp) asm volatile("mov sp, %0     \n" ::"r"(sp));
//     asm volatile("eret  \n");
// }


void
initrd_exec_replace(const byteptr_t name)
{
    finfo_t info;
    byteptr_t file = _initrd_find_file(name, &info);
    if (file) { 
        // uart_printf("initrd_exec_replace - info: %s, size: 0x%x, ", info.name, info.size);
        // int32_t pid = 
        thread_replace_user((uint64_t) 0, (uint64_t) info.content, info.size);
        // uart_printf("pid = %d\n", pid);
        run_user_process((uint64_t) current_task()->user_prog);
    }
}


void 
initrd_traverse(cpio_cb * cb)
{
    finfo_t info;
    byteptr_t cpio_addr = initrd_get_ptr();
    byteptr_t curr = (byteptr_t) cpio_addr;
    byteptr_t next = _initrd_next(cpio_addr, &info);

    while (next) {
        (*cb)(info.name, info.content, info.size);
        curr = next;
        next = _initrd_next(curr, &info);
    }
}