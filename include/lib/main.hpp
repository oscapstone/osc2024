#pragma once

extern "C" {
extern char __stack_end[];

void _start();
void bootloader_main(void* dtb_addr);
void kernel_main(void* dtb_addr);
}
