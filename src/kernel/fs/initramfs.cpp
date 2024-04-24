#include "fs/initramfs.hpp"

#include "fdt.hpp"
#include "io.hpp"

CPIO initramfs;

void initramfs_init() {
  auto find32 = [](auto path) {
    auto [found, view] = fdt.find(path);
    if (!found) {
      klog("initramfs: device %s not found\n", path);
      prog_hang();
    }
    auto addr = (char*)(uint64_t)fdt_ld32(view.data());
    return addr;
  };
  auto start = find32("/chosen/linux,initrd-start");
  auto end = find32("/chosen/linux,initrd-end");

  klog("initramfs      : %p ~ %p\n", start, end);
  if (not initramfs.init(start, end)) {
    klog("initramfs: init failed\n");
  }
}
