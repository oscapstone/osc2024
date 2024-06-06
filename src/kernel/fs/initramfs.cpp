#include "fs/initramfs.hpp"

#include "fdt.hpp"
#include "fs/log.hpp"
#include "fs/vfs.hpp"
#include "io.hpp"
#include "mm/mmu.hpp"

#define FS_TYPE "initramfs"

namespace initramfs {

CPIO cpio;

void preinit() {
  auto find32 = [](auto path) {
    auto [found, view] = fdt.find(path);
    if (not found)
      panic("initramfs: device %s not found", path);
    auto addr = pa2va((char*)(uint64_t)fdt_ld32(view.data()));
    return addr;
  };
  auto start = find32("/chosen/linux,initrd-start");
  auto end = find32("/chosen/linux,initrd-end");

  klog("initramfs      : %p ~ %p\n", start, end);
  if (not cpio.init(start, end)) {
    klog("initramfs: init failed\n");
  }
}

::FileSystem* init() {
  static FileSystem* fs = nullptr;
  if (not fs)
    fs = new FileSystem;
  return fs;
}

FileSystem::FileSystem() {
  root = new ::Vnode{kDir};
  for (auto hdr : cpio) {
    ::Vnode* dir;
    char* basename;
    if (vfs_lookup(root, hdr->name_ptr(), dir, basename) < 0) {
      FS_WARN("lookup '%s' fail", hdr->name_ptr());
      continue;
    }
    if (not dir->lookup(basename))
      dir->link(basename, new Vnode{hdr});
    kfree(basename);
  }
}

};  // namespace initramfs
