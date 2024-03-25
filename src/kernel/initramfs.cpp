#include "initramfs.hpp"

#include "board/mini-uart.hpp"
#include "fdt.hpp"

CPIO initramfs;
const char initrd_path[] = "/chosen/linux,initrd-start";

void initramfs_init() {
  auto [found, view] = fdt.find(initrd_path);
  if (!found) {
    mini_uart_printf("initramfs init failed, %s not found\n", initrd_path);
    prog_hang();
  }
  auto addr = (char*)(uint64_t)fdt_ld32((uint32_t*)view.data());
  mini_uart_printf("initramfs: %p\n", addr);
  initramfs.init(addr);
}
