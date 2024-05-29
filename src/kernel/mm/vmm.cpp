#include "mm/vmm.hpp"

#include "io.hpp"
#include "mm/mmu.hpp"
#include "mm/vsyscall.hpp"
#include "string.hpp"
#include "syscall.hpp"
#include "thread.hpp"

SYSCALL_DEFINE6(mmap, void*, addr, size_t, len, int, prot, int, flags, int, fd,
                int, file_offset) {
  klog("mmap(%p, 0x%lx, %b, %b, %d, 0x%x)\n", addr, len, prot, flags, fd,
       file_offset);
  return current_thread()->vmm.mmap((uint64_t)addr, len,
                                    cast_enum<ProtFlags>(prot),
                                    cast_enum<MmapFlags>(flags), nullptr);
}

VMM::VMM(const VMM& o)
    : el0_tlb(pt_copy(o.el0_tlb)),
      vmas{o.vmas},
      user_ro_pages{o.user_ro_pages} {}

VMM::~VMM() {
  reset();
}

void VMM::ensure_el0_tlb() {
  if (el0_tlb == nullptr) {
    el0_tlb = new PT;
    load_tlb(el0_tlb);
    auto vsyscall_addr =
        map_user_phy_pages(VSYSCALL_START, (uint64_t)__vsyscall_beg, PAGE_SIZE,
                           ProtFlags::RX, "[vsyscall]]");
    if (vsyscall_addr != VSYSCALL_START)
      panic("vsyscall addr shouldn't change!");
  }
}

bool VMM::vma_overlap(uint64_t va, uint64_t size) {
  for (auto& vma : vmas)
    if (vma.overlap(va, size))
      return true;
  return false;
}

uint64_t VMM::vma_addr(uint64_t va, uint64_t size) {
  if (not vma_overlap(va, size))
    return va;

  auto it = vmas.begin();
  while (not it->overlap(va, size))
    it++;

  auto close_to_start = va - USER_SPACE_START < USER_SPACE_END - va;
  auto next = close_to_start ? +[](decltype(it) it) { return ++it; }
                             : +[](decltype(it) it) { return --it; };
  auto end = close_to_start ? vmas.end() : vmas.head();

  while (it != end) {
    auto nva = close_to_start ? it->end() : it->start() - size;
    auto nxt = next(it);
    if (nxt == end or not nxt->overlap(nva, size))
      return nva;
    it = nxt;
  }

  return INVALID_ADDRESS;
}

void VMM::vma_add(string name, uint64_t addr, uint64_t size, ProtFlags prot) {
  auto it = vmas.begin();
  auto end = vmas.end();
  while (it != end and it->end() <= addr)
    it++;
  vmas.insert_before(it, new VMA{name, addr, size, prot});
  klog("vma_add: 0x%016lx ~ 0x%016lx %s\n", addr, addr + size, name.data());
}

VMA* VMM::vma_find(uint64_t va) {
  for (auto& vma : vmas)
    if (vma.contain(va))
      return &vma;
  return nullptr;
}

void VMM::vma_print() {
  klog("==== maps ====\n");
  klog("vma size = %d\n", vmas.size());
  for (auto& vma : vmas)
    klog("0x%016lx ~ 0x%016lx %s\n", vma.start(), vma.end(), vma.name.data());
  klog("--------------\n");
}

uint64_t VMM::mmap(uint64_t va, uint64_t size, ProtFlags prot, MmapFlags flags,
                   const char* name) {
  ensure_el0_tlb();

  size = align<PAGE_SIZE>(size);
  va = align<PAGE_SIZE, false>(va);

  va = vma_addr(va, size);
  if (va == INVALID_ADDRESS)
    return INVALID_ADDRESS;

  vma_add(name ? name : "[anon_" + to_hex_string(va) + "]", va, size, prot);

  el0_tlb->walk(
      va, va + size,
      [](auto context, PT_Entry& entry, auto start, auto level) {
        auto prot = cast_enum<ProtFlags>(context);
        // TODO: demand paging
        entry.alloc(level);

        if (has(prot, ProtFlags::WRITE))
          entry.AP = AP::USER_RW;
        else
          entry.AP = AP::USER_RO;
        entry.UXN = not has(prot, ProtFlags::EXEC);

        klog("mmap:  0x%016lx ~ 0x%016lx -> ", start,
             start + ENTRY_SIZE[level]);
        entry.print(level);
      },
      (void*)prot);

  reload_tlb();

  return va;
}

uint64_t VMM::map_user_phy_pages(uint64_t va, uint64_t pa, uint64_t size,
                                 ProtFlags prot, const char* name) {
  ensure_el0_tlb();

  size = align<PAGE_SIZE>(size);
  va = align<PAGE_SIZE, false>(va);

  va = vma_addr(va, size);
  if (va == INVALID_ADDRESS)
    return INVALID_ADDRESS;

  vma_add(name ? name : "[phy_" + to_hex_string(pa) + "]", va, size, prot);

  struct Ctx {
    uint64_t va;
    uint64_t pa;
    ProtFlags prot;
  } ctx{
      .va = va,
      .pa = pa,
      .prot = prot,
  };

  klog("map_user_phy_pages:  0x%016lx ~ 0x%016lx -> %08lx\n", va, va + size,
       pa);

  // TODO: handle address overlap
  el0_tlb->walk(
      va, va + size,
      [](auto context, PT_Entry& entry, auto start, auto level) {
        auto ctx = (Ctx*)context;
        entry.alloc(level);

        entry.set_entry(ctx->pa + (start - ctx->va), level);

        if (has(ctx->prot, ProtFlags::WRITE))
          entry.AP = AP::USER_RW;
        else
          entry.AP = AP::USER_RO;
        entry.UXN = not has(ctx->prot, ProtFlags::EXEC);
      },
      (void*)&ctx);

  reload_tlb();

  return va;
}

void VMM::reset() {
  if (el0_tlb) {
    delete el0_tlb;
    el0_tlb = nullptr;
  }
  vmas.clear();
  user_ro_pages.clear();
}

void VMM::return_to_user() {
  for (auto& page : user_ro_pages) {
    auto entry = el0_tlb->get_entry(page.addr);
    if (entry)
      entry->AP = AP::USER_RO;
  }
  user_ro_pages.clear();
  reload_tlb();
}
