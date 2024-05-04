#include "exec.hpp"

#include "fs/initramfs.hpp"
#include "io.hpp"
#include "mm/mm.hpp"

int exec(const char* name, char* const argv[]) {
  klog("%s(%s, %p)\n", __func__, name, argv);

  auto hdr = initramfs.find(name);
  if (hdr == nullptr) {
    klog("%s: %s: No such file or directory\n", __func__, name);
    return -1;
  }
  if (hdr->isdir()) {
    klog("%s: %s: Is a directory\n", __func__, name);
    return -1;
  }

  auto file = hdr->file();
  auto user_text = kmalloc(file.size(), PAGE_SIZE);
  if (user_text == nullptr) {
    klog("%s: can't alloc user_text / size = %x\n", __func__, file.size());
    return -1;
  }
  auto user_stack = kmalloc(USER_STACK_SIZE, PAGE_SIZE);
  if (user_stack == nullptr) {
    kfree(user_text);
    klog("%s: can't alloc user_stack / size = %lx\n", __func__,
         USER_STACK_SIZE);
    return -1;
  }

  memcpy(user_text, file.data(), file.size());
  exec_user_prog(user_text, user_stack);

  return 0;
}
