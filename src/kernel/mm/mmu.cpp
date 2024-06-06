#include "mm/mmu.hpp"

#include "io.hpp"
#include "mm/page.hpp"
#include "sched.hpp"
#include "thread.hpp"

#define MMU_DEBUG 0

void PT_Entry::print(int level) const {
  kprintf("0x%08lx Attr%d %s %s", (uint64_t)addr(), AttrIdx, PT_levelstr(level),
          kindstr());
  if (isEntry()) {
    kprintf(" %s", apstr());
    if (UXN)
      kprintf(" UXN");
    if (PXN)
      kprintf(" PXN");
    if (AF)
      kprintf(" AF");
  }
  kprintf("\n");
}

void PT_Entry::alloc_table(int level) {
  set_level(level);
  int nxt_level = level + 1;
  auto nxt_table = new PT(*this, nxt_level);
  set_table(level, nxt_table);
}

void PT_Entry::alloc_user_page(ProtFlags prot) {
  set_user_entry(kcalloc(PAGE_SIZE), PTE_LEVEL, prot, true);
}

void PT_Entry::dealloc_page() {
  if (not isEntry())
    return;
  if (require_free) {
    auto va = addr_va();
    auto& ref = mm_page.refcnt(va);
    if (--ref == 0)
      kfree(va);
  }
}

PT_Entry PT_Entry::copy(int level) {
  auto new_entry = *this;
  switch (kind()) {
    case EntryKind::TABLE:
      // TODO: copy on write
      new_entry.set_table(level, pt_copy(table()));
      break;
    case EntryKind::ENTRY: {
      // copy on write
      AP = AP::USER_RO;
      new_entry.AP = AP::USER_RO;
      auto& ref = mm_page.refcnt(addr_va());
      ref++;
      break;
    }
    case EntryKind::INVALID:
      // just copy invalid entry
      break;
  }
  return new_entry;
}

void PT_Entry::copy_on_write() {
  if (not mm_page.managed(addr_va())) {
    // assume peripheral memory
  } else {
    auto& ref = mm_page.refcnt(addr_va());
    if (ref > 1) {
      ref--;
      auto new_page = kmalloc(PAGE_SIZE);
      memcpy(new_page, addr_va(), PAGE_SIZE);
      set_addr(va2pa(new_page), type);
    }
  }
  AP = AP::USER_RW;
}

PT::PT() {
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = {
        .AF = false,
        .type = PD_INVALID,
    };
  }
}

PT::PT(PT_Entry entry, int level) {
  entry.set_level(level);
  uint64_t offset = ENTRY_SIZE[level] / PAGE_SIZE;
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = entry;
    entry.output_address += offset;
  }
}

PT* pt_copy(PT* o) {
  if (o == nullptr)
    return nullptr;
  auto npt = new PT(o);
  reload_pgd();
  return npt;
}

PT::PT(PT* o, int level) {
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = o->entries[i].copy(level);
  }
}

PT::~PT() {
  this->traverse([](auto, auto entry, auto, auto) { entry.dealloc_page(); },
                 [](auto, auto entry, auto, auto) { delete entry.table(); });
}

PT_Entry& PT::walk(uint64_t start, int level, uint64_t va_start, int va_level) {
  if (addressSpace(start) != addressSpace(va_start))
    panic("page table walk address space mismatch");
  uint64_t idx = (va_start - start) / ENTRY_SIZE[level];
  auto& entry = entries[idx];
  if (level == va_level or entry.isPTE()) {
    return entry;
  }
  auto nxt_start = start + idx * ENTRY_SIZE[level];
  auto nxt_level = level + 1;
  if (not entry.isTable())
    entry.alloc_table(level);
  PT* nxt_table = entry.table();
  return nxt_table->walk(nxt_start, nxt_level, va_start, va_level);
}

void PT::walk(uint64_t start, int level, uint64_t va_start, uint64_t va_end,
              int va_level, CB cb_entry, void* context) {
  va_start = align<PAGE_SIZE, false>(va_start);
  va_end = align<PAGE_SIZE>(va_end);
  while (va_start % ENTRY_SIZE[va_level] != 0 or
         va_end % ENTRY_SIZE[va_level] != 0)
    va_level++;
  for (auto it = va_start; it < va_end; it += ENTRY_SIZE[va_level]) {
    auto& entry = walk(start, level, it, va_level);
    cb_entry(context, entry, it, va_level);
  }
}

PT_Entry* PT::get_entry(uint64_t start, int level, uint64_t addr, bool alloc) {
  uint64_t idx = (addr - start) / ENTRY_SIZE[level];
  auto entry = &entries[idx];

  if (level == PTE_LEVEL)
    return entry;

  if (alloc and entry->isInvalid())
    entry->alloc_table(level);

  if (entry->kind() == EntryKind::TABLE) {
    auto nxt_start = start + idx * ENTRY_SIZE[level];
    auto nxt_level = level + 1;
    return entry->table()->get_entry(nxt_start, nxt_level, addr, alloc);
  }
  return nullptr;
}

void PT::traverse(uint64_t start, int level, CB cb_entry, CB cb_table,
                  void* context) {
  auto nxt_start = start;
  auto nxt_level = level + 1;
  for (uint64_t idx = 0; idx < TABLE_SIZE_4K; idx++) {
    auto& entry = entries[idx];
    switch (entry.kind()) {
      case EntryKind::TABLE:
        if (cb_table)
          cb_table(context, entry, nxt_start, nxt_level);
        else
          entry.table()->traverse(nxt_start, nxt_level, cb_entry, cb_table,
                                  context);
        break;
      case EntryKind::ENTRY:
        cb_entry(context, entry, nxt_start, level);
        break;
      case EntryKind::INVALID:
        break;
    }
    nxt_start += ENTRY_SIZE[level];
  }
}

void PT::print(const char* name, uint64_t start, int level, uint64_t pad) {
  if (pad == 0)
    kprintf("===== %s ===== @ %p\n", name, va2pa(this));
  traverse(
      start, level,
      [](auto ctx, auto entry, auto start, auto level) {
        auto pad = (uint64_t)ctx;
        for (uint64_t i = 0; i < pad; i++)
          kputc(' ');
        kprintf("%016lx ~ %016lx -> ", start, start + ENTRY_SIZE[level]);
        entry.print(level);
      },
      [](auto ctx, auto entry, auto start, auto level) {
        auto pad = (uint64_t)ctx;
        for (uint64_t i = 0; i < pad; i++)
          kputc(' ');
        kprintf("%016lx ~ %016lx -> ", start, start + ENTRY_SIZE[level]);
        entry.print(level);
        entry.table()->print(nullptr, start, level, pad + 1);
      },
      (void*)pad);
  if (pad == 0)
    kprintf("----------------------\n");
}

void* PT::translate_va(uint64_t va, uint64_t start, int level) {
  uint64_t idx = (va - start) / ENTRY_SIZE[level];
  auto nxt_start = start + idx * ENTRY_SIZE[level];
  auto nxt_level = level + 1;

  auto& entry = entries[idx];

  if (false) {
    kprintf("translate_va %s 0x%lx: 0x%lx -> ", PT_levelstr(level), start, va);
    entry.print(level);
    kprintf("\n");
  }

  if (entry.isEntry())
    return entry.addr(va % ENTRY_SIZE[level]);

  return entry.table()->translate_va(va, nxt_start, nxt_level);
}

void map_kernel_as_normal(char* ktext_beg, char* ktext_end) {
  PT_Entry PMD_entry{
      .type = PD_BLOCK,
      .AttrIdx = MAIR_IDX_DEVICE_nGnRnE,
      .AP = AP::KERNEL_RW,
      .AF = true,
      .output_address = MEM_START / PAGE_SIZE,
      .PXN = true,
      .UXN = true,
      .level_is_PTE = false,
  };
  auto PMD = new PT(PMD_entry, PMD_LEVEL);

  PMD->walk(KERNEL_SPACE, PMD_LEVEL, mm_page.start(), mm_page.end(), PMD_LEVEL,
            [](auto, auto entry, auto, auto) {
              entry.AttrIdx = MAIR_IDX_NORMAL_NOCACHE;
            });

  PMD->walk(KERNEL_SPACE, PMD_LEVEL, (uint64_t)ktext_beg, (uint64_t)ktext_end,
            PMD_LEVEL, [](auto, auto entry, auto, auto) {
              entry.AP = AP::KERNEL_RO;
              entry.PXN = false;
            });

  PMD->traverse(KERNEL_SPACE, PMD_LEVEL,
                [](auto, auto entry, auto, auto) { entry.UXN = true; });

  auto PUD = (PT*)__upper_PUD;
  PUD->set_level(PUD_LEVEL);
  auto PGD = (PT*)__upper_PGD;
  PGD->set_level(PGD_LEVEL);

  PUD->entries[0].set_table(PUD_LEVEL, PMD);
  PUD->entries[1].PXN = true;
  PUD->entries[1].UXN = true;

  reload_pgd();
}

struct IIS_DABT {
  uint32_t DFSC : 4;
  uint32_t WnR : 1;
  uint32_t S1PTW : 1;
  uint32_t CM : 1;
  uint32_t EA : 1;
  uint32_t FnV : 1;
  uint32_t LST : 2;
  uint32_t VNCR : 1;
  uint32_t AR : 1;
  uint32_t SF : 1;
  uint32_t SRT : 5;
  uint32_t SSE : 1;
  uint32_t SAS : 2;
  uint32_t ISV : 1;
};

int fault_handler(int el) {
  unsigned long esr = read_sysreg(ESR_EL1);
  auto iss = (IIS_DABT)ESR_ELx_ISS(esr);
  auto tid = current_thread()->tid;

  if (iss.FnV) {
    klog("t%d fault error: FAR is not valid\n", tid);
    return -1;
  }

  auto faddr = (void*)read_sysreg(FAR_EL1);

  if (isKernelSpace(faddr)) {
    klog("t%d fault error: in kernel space\n", tid);
    return -1;
  }

  auto fpage = getPage(faddr);
  auto entry = current_vmm()->ttbr0->get_entry(fpage);
  auto vma = find_vma(faddr);

  switch (iss.DFSC) {
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L0:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L1:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L2:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L3:
#if MMU_DEBUG > 0
      klog("t%d [Translation fault]: %016lx\n", tid, (uint64_t)faddr);
#endif
      if (!vma)
        return -1;
      entry = current_vmm()->ttbr0->get_entry(fpage, true);
      entry->alloc_user_page(vma->prot);
      break;

    case ESR_ELx_IIS_DFSC_PERM_FAULT_L3:
      if (el == 1) {
        entry->AP = AP::KERNEL_RW;
        current_vmm()->user_ro_pages.push_back(fpage);
      } else if (vma and has(vma->prot, ProtFlags::WRITE)) {
        // copy on write fault
#if MMU_DEBUG > 0
        klog("t%d [CoW fault]: %016lx\n", tid, (uint64_t)faddr);
#endif
        entry->copy_on_write();
      } else {
        return -1;
      }
      break;

    default:
      klog("t%d unknown DFSC %06b\n", tid, iss.DFSC);
      return -1;
  }

  reload_pgd();
  return 0;
}
