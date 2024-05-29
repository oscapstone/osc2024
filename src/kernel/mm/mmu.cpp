#include "mm/mmu.hpp"

#include "io.hpp"
#include "mm/page.hpp"
#include "sched.hpp"
#include "thread.hpp"

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

PT_Entry PT_Entry::copy(int level) const {
  auto new_entry = *this;
  // TODO: copy on write
  switch (kind()) {
    case EntryKind::TABLE:
      new_entry.set_table(level, pt_copy(table()));
      break;
    case EntryKind::ENTRY: {
      auto new_page = kmalloc(PAGE_SIZE);
      memcpy(new_page, pa2va(addr()), PAGE_SIZE);
      new_entry.set_addr(va2pa(new_page), type);
      break;
    }
    case EntryKind::INVALID:
      // just copy invalid entry
      break;
  }
  return new_entry;
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
  return new PT(o);
}

PT::PT(PT* o, int level) {
  // TODO: copy on write
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = o->entries[i].copy(level);
  }
}

PT::~PT() {
  this->traverse(
      [](auto, auto entry, auto, auto) {
        if (entry.require_free)
          kfree(pa2va(entry.addr()));
      },
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

void PT::print(const char* name, uint64_t start, int level) {
  kprintf("===== %s ===== @ %p\n", name, va2pa(this));
  traverse(start, level, [](auto, auto entry, auto start, auto level) {
    kprintf("%016lx ~ %016lx -> ", start, start + ENTRY_SIZE[level]);
    entry.print(level);
  });
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

  if (iss.FnV) {
    klog("fault error: FAR is not valid\n");
    return -1;
  }

  auto faddr = (void*)read_sysreg(FAR_EL1);

  if (isKernelSpace(faddr)) {
    klog("fault error: in kernel space\n");
    return -1;
  }

  auto fpage = getPage(faddr);
  auto entry = current_thread()->vmm.el0_pgd->get_entry(fpage);
  auto vma = find_vma(faddr);

  switch (iss.DFSC) {
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L0:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L1:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L2:
    case ESR_ELx_IIS_DFSC_TRAN_FAULT_L3:
      klog("[Translation fault]: %016lx\n", (uint64_t)faddr);
      if (!vma)
        return -1;
      entry = current_thread()->vmm.el0_pgd->get_entry(fpage, true);
      entry->alloc_user_page(vma->prot);
      reload_pgd();
      return 0;

    case ESR_ELx_IIS_DFSC_PERM_FAULT_L3:
      if (el == 1) {
        entry->AP = AP::KERNEL_RW;
        current_thread()->vmm.user_ro_pages.push_back(new PageItem{fpage});
        return 0;
      } else {
        return -1;
      }

    default:
      kprintf("unknown DFSC %06b\n", iss.DFSC);
      return -1;
  }
}
