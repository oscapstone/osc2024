#include "exec.hpp"

#include "fs/initramfs.hpp"
#include "io.hpp"
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

  thread->reset_el0_tlb();

  // TODO: RO TEXT
  if (thread->alloc_user_pages(USER_TEXT_START, file.size(), ProtFlags::RWX)) {
    klog("%s: can't alloc user_text for thread %d / size = %lx\n", __func__,
         thread->tid, file.size());
    return -1;
  }
  if (thread->alloc_user_pages(USER_STACK_START, USER_STACK_SIZE,
                               ProtFlags::RW)) {
    klog("%s: can't alloc user_stack for thread %d / size = %lx\n", __func__,
         thread->tid, USER_STACK_SIZE);
    return -1;
  }

  delete ctx;

  memcpy((void*)USER_TEXT_START, file.data(), file.size());
  memzero((void*)USER_STACK_START, (void*)USER_STACK_END);
  thread->reset_kernel_stack();

  exec_user_prog((void*)USER_TEXT_START, (void*)USER_STACK_END,
                 thread->regs.sp);

  return 0;
}

void exec_new_user_prog(void* ctx) {
  exec((ExecCtx*)ctx);
}
