#pragma once

#include <cstdint>

extern "C" {
void _start();
void bootloader_main(void* dtb_addr);
void kernel_main(void* dtb_addr, uint32_t size);
}
