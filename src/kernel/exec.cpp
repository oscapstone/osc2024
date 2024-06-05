#include "exec.hpp"

#include "io.hpp"
#include "mm/vmm.hpp"
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

  auto vnode = vfs_lookup(name);
  if (vnode == nullptr) {
    klog("%s: %s: No such file or directory\n", __func__, name);
    return -1;
  }
  if (vnode->isDir()) {
    klog("%s: %s: Is a directory\n", __func__, name);
    return -1;
  }

  File* file;
  if (vfs_open(name, O_RDONLY, file) < 0) {
    klog("%s: %s: can't open\n", __func__, name);
    return -1;
  }
  auto thread = current_thread();

  thread->vmm.reset();

  // TODO: map from fs
  auto text_addr = mmap(USER_TEXT_START, file->size(), ProtFlags::RX,
                        MmapFlags::MAP_ANONYMOUS, name);
  if (text_addr == MAP_FAILED) {
    klog("%s: can't alloc user_text for thread %d / size = %lx\n", __func__,
         thread->tid, file->size());
    return -1;
  }

  auto stack_addr = mmap(USER_STACK_START, USER_STACK_SIZE, ProtFlags::RW,
                         MmapFlags::MAP_ANONYMOUS, "[stack]");
  if (stack_addr == MAP_FAILED) {
    klog("%s: can't alloc user_stack for thread %d / size = %lx\n", __func__,
         thread->tid, USER_STACK_SIZE);
    return -1;
  }
  auto stack_end = stack_addr + USER_STACK_SIZE;

  delete ctx;

  file->read((void*)text_addr, file->size());
  file->close();
  thread->reset_kernel_stack();

  thread->vmm.return_to_user();
  exec_user_prog((void*)text_addr, (void*)stack_end, thread->regs.sp);

  return 0;
}

void exec_new_user_prog(void* ctx) {
  exec((ExecCtx*)ctx);
}
