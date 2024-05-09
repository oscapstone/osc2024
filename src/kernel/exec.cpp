#include "exec.hpp"

#include "fs/initramfs.hpp"
#include "io.hpp"
#include "string.hpp"
#include "thread.hpp"

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
  auto thread = current_thread();
  if (thread->alloc_user_text_stack(file.size(), USER_STACK_SIZE)) {
    return -1;
  }

  memcpy(thread->user_text.addr, file.data(), file.size());
  thread->reset_kernel_stack();
  exec_user_prog(thread->user_text.addr, thread->user_stack.end(0x10),
                 thread->regs.sp);

  return 0;
}

void exec_new_user_prog(void* ctx) {
  auto name = (char*)ctx;
  // TODO: mem leak
  exec(name, nullptr);
  // kfree(ctx);
}
