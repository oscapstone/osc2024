#pragma once

extern "C" {
void _start();
void bootloader_main(void* dtb_addr);
void kernel_main(void* dtb_addr);
}
