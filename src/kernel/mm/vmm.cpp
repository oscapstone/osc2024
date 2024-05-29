#include "mm/vmm.hpp"

#include "io.hpp"
#include "mm/mmu.hpp"
#include "mm/vsyscall.hpp"

VMM::VMM(const VMM& o)
    : el0_tlb(pt_copy(o.el0_tlb)),
      items{o.items},
      user_ro_pages{o.user_ro_pages} {}

VMM::~VMM() {
  reset();
}

void VMM::ensure_el0_tlb() {
  if (el0_tlb == nullptr) {
    el0_tlb = new PT;
    load_tlb(el0_tlb);
    map_user_phy_pages(VSYSCALL_START, (uint64_t)__vsyscall_beg, PAGE_SIZE,
                       ProtFlags::RX);
  }
}

int VMM::alloc_user_pages(uint64_t va, uint64_t size, ProtFlags prot) {
  ensure_el0_tlb();

  // TODO: handle address overlap
  el0_tlb->walk(
      va, va + size,
      [](auto context, PT_Entry& entry, auto start, auto level) {
        auto prot = cast_enum<ProtFlags>(context);
        entry.alloc(level);

        if (has(prot, ProtFlags::WRITE))
          entry.AP = AP::USER_RW;
        else
          entry.AP = AP::USER_RO;
        entry.UXN = not has(prot, ProtFlags::EXEC);

        klog("alloc_user_pages:  0x%016lx ~ 0x%016lx -> ", start,
             start + ENTRY_SIZE[level]);
        entry.print(level);
      },
      (void*)prot);

  reload_tlb();

  return 0;
}

int VMM::map_user_phy_pages(uint64_t va, uint64_t pa, uint64_t size,
                            ProtFlags prot) {
  ensure_el0_tlb();

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

  return 0;
}

void VMM::reset() {
  if (el0_tlb) {
    delete el0_tlb;
    el0_tlb = nullptr;
  }
  items.clear();
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
