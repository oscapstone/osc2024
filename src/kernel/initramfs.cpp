#include "initramfs.hpp"

#include "board/mini-uart.hpp"
#include "fdt.hpp"

CPIO initramfs;

void initramfs_init() {
  auto view = fdt.find("/chosen/linux,initrd-start");
  auto addr = (char*)(uint64_t)fdt_ld32((uint32_t*)view.data());
  mini_uart_printf("initramfs: %p\n", addr);
  initramfs.init(addr);
}
