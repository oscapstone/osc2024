#include "mm/mmu.hpp"

#include "io.hpp"
#include "mm/page.hpp"

void PageTableEntry::print() const {
  if (UXN)
    kprintf("UXN ");
  if (PXN)
    kprintf("PXN ");
  if (AF)
    kprintf("AF ");
  if (RDONLY)
    kprintf("RDONLY ");
  if (USER)
    kprintf("USER ");
  kprintf("addr %p Attr %d type %d\n", addr(), AttrIdx, type);
}

PageTable::PageTable() {
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = {
        .AF = false,
        .type = PD_INVALID,
    };
  }
}

PageTable::PageTable(PageTableEntry entry, int level) {
  if (level == PTE_LEVEL and entry.type == PD_BLOCK)
    entry.type = PD_ENTRY;
  uint64_t offset = ENTRY_SIZE[level] / PAGE_SIZE;
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = entry;
    entry.output_address += offset;
  }
}

PageTableEntry& PageTable::walk(uint64_t start, int level, uint64_t va_start,
                                int va_level) {
  if (addressSpace(start) != addressSpace(va_start))
    panic("page table walk address space mismatch");
  uint64_t idx = (va_start - start) / ENTRY_SIZE[level];
  auto& entry = entries[idx];
  if (level == va_level) {
    return entry;
  }
  auto nxt_start = start + idx * ENTRY_SIZE[level];
  auto nxt_level = level + 1;
  PageTable* nxt_table;
  if (entry.isTable()) {
    nxt_table = entry.table();
  } else {
    nxt_table = new PageTable(entry, nxt_level);
    entry.set_table(nxt_table);
  }
  return nxt_table->walk(nxt_start, nxt_level, va_start, va_level);
}

void PageTable::walk(uint64_t start, int level, uint64_t va_start,
                     uint64_t va_end, int va_level, CB cb_entry,
                     void* context) {
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

void PageTable::traverse(uint64_t start, int level, CB cb_entry, CB cb_table,
                         void* context) {
  auto nxt_start = start;
  auto nxt_level = level + 1;
  for (uint64_t idx = 0; idx < TABLE_SIZE_4K; idx++) {
    auto& entry = entries[idx];
    switch (entry.type) {
      case PD_TABLE:
        if (cb_table) {
          cb_table(context, entry, nxt_start, nxt_level);
        } else {
          entry.table()->traverse(nxt_start, nxt_level, cb_entry, cb_table);
        }
        break;
      case PD_BLOCK:
      case PD_ENTRY:
        cb_entry(context, entry, nxt_start, level);
        break;
    }
    nxt_start += ENTRY_SIZE[level];
  }
}

void PageTable::print(const char* name, uint64_t start, int level) {
  kprintf("===== %s ===== \n", name);
  traverse(start, level, [](auto, auto entry, auto start, auto level) {
    kprintf("%lx ~ %lx: ", start, start + ENTRY_SIZE[level]);
    entry.print();
  });
  kprintf("----------------------\n");
}

// TODO: remove -mno-unaligned-access
void map_kernel_as_normal(char* ktext_beg, char* ktext_end) {
  PageTableEntry PMD_entry{
      .PXN = false,
      .output_address = 0,
      .RDONLY = false,
      .USER = false,
      .AttrIdx = MAIR_IDX_DEVICE_nGnRnE,
      .type = PD_BLOCK,
  };
  auto PMD = new PageTable(PMD_entry, PMD_LEVEL);

  PMD->walk(KERNEL_SPACE, PMD_LEVEL, mm_page.start(), mm_page.end(), PMD_LEVEL,
            [](auto, auto entry, auto, auto) {
              entry.AttrIdx = MAIR_IDX_NORMAL_NOCACHE;
            });

  PMD->walk(KERNEL_SPACE, PMD_LEVEL, (uint64_t)ktext_beg, (uint64_t)ktext_end,
            PMD_LEVEL,
            [](auto, auto entry, auto, auto) { entry.RDONLY = true; });

  PMD->traverse(KERNEL_SPACE, PMD_LEVEL,
                [](auto, auto entry, auto, auto) { entry.PXN = true; });

  auto PUD = (PageTable*)__upper_PUD;
  PUD->entries[0].set_table(PMD);
  PUD->entries[1].PXN = true;

  asm volatile(
      "dsb ISH\n"         // ensure write has completed
      "tlbi VMALLE1IS\n"  // invalidate all TLB entries
      "dsb ISH\n"         // ensure completion of TLB invalidatation
      "isb\n"             // clear pipeline
  );
}
