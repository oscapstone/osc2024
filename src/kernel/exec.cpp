#include "exec.hpp"

#include "fs/initramfs.hpp"
#include "io.hpp"
#include "string.hpp"
#include "syscall.hpp"
#include "thread.hpp"

SYSCALL_DEFINE2(exec, const char*, name, char** const, argv) {
  auto ctx = new ExecCtx{name, argv};
  return exec(ctx);
}

int exec(ExecCtx* ctx) {
  // TODO: args, envp
  auto name = ctx->name;

  klog("%s(%s)\n", __func__, name);

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

  delete ctx;

  memcpy(thread->user_text.addr, file.data(), file.size());
  thread->reset_kernel_stack();
  thread->user_stack.clean();

  klog("exec_user_prog: text %p stack %p\n", thread->user_text.addr,
       thread->user_stack.end(0x10));
  exec_user_prog(thread->user_text.addr, thread->user_stack.end(0x10),
                 thread->regs.sp);

  return 0;
}

void exec_new_user_prog(void* ctx) {
  exec((ExecCtx*)ctx);
}
