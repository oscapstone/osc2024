#include "fs/initramfs.hpp"

#include "fdt.hpp"
#include "io.hpp"

CPIO initramfs;
const char initrd_path[] = "/chosen/linux,initrd-start";

void initramfs_init() {
  auto [found, view] = fdt.find(initrd_path);
  if (!found) {
    klog("initramfs: device %s not found\n", initrd_path);
    prog_hang();
  }
  auto addr = (char*)(uint64_t)fdt_ld32(view.data());
  klog("initramfs: %p\n", addr);
  if (not initramfs.init(addr)) {
    klog("initramfs: init failed\n");
  }
}
