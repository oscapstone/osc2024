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

::Vnode* FileSystem::mount(const ::Mount* mount_root) {
  auto root = new ::Vnode{mount_root, kDir};
  if (not root)
    return nullptr;
  for (auto hdr : cpio) {
    ::Vnode* dir;
    char* basename;
    if (vfs_lookup(root, hdr->name_ptr(), dir, basename) < 0) {
      FS_WARN("lookup '%s' fail", hdr->name_ptr());
      continue;
    }
    if (not dir->lookup(basename))
      dir->link(basename, new Vnode{mount_root, hdr});
    kfree(basename);
  }
  return root;
}

};  // namespace initramfs
