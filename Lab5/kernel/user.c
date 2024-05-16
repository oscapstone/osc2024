#include "initramdisk.h"
#include "user.h"
#include "../peripherals/mini_uart.h"
#include "../peripherals/mm.h"

#define USER_STACK_SIZE 0x2000

int load_user_prog(uintptr_t file_addr) {
    uintptr_t user_start = 0x100000;
    uintptr_t user_sp = user_start + USER_STACK_SIZE;
    
    // Load the file content into memory.
    struct File* file = (struct File *)file_addr;
    unsigned int fileSize = file->fileSize;

    // Write the user program into the address specified by the linker.
    char* userProg = (char *)0x100000;

    int i = 0;
    while (fileSize--) {
        uart_send(file->fData[i]);
        *userProg++ = file->fData[i++];
    }

    uart_send_string("Print file size\r\n");
    uart_send_uint((uintptr_t)file);
    uart_send_string("\r\n");
    uart_send_uint(file->fileSize);
    uart_send_string("\r\n");
    
    
    // EL1 to EL0.
    asm volatile (
        // Set SPSR_EL1 to 0x3C0 to disable interrupts and set appropriate state for EL0.
        // [9:6]DAIF -> Enable interrupt in user program.(I -> 0)
        "mov x0, 0x340;"
        "msr spsr_el1, x0;"

        // Set ELR_EL1 to the start address of the user program.
        "mov x1, %0;"
        "msr elr_el1, x1;"

        // Set SP_EL0 to a valid stack pointer for the user program.
        "mov x2, %1;"
        "msr sp_el0, x2;"
        "eret;"

        // No output operands.
        :
        // Input operands.
        : "r" (user_start), "r" (user_sp)
        // List of clobbered registers.(Tells the compiler which registers the assembly code modifies or "clobbers.")
        : "x0", "x1", "x2"
    );
    
}