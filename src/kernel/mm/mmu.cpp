#include "mm/mmu.hpp"

#include "mm/page.hpp"

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
  PageTable* nxt_table = new PageTable(entry, nxt_level);
  entry.set_table(nxt_table);
  return nxt_table->walk(nxt_start, nxt_level, va_start, va_level);
}

void PageTable::walk(uint64_t start, int level, uint64_t va_start,
                     uint64_t va_end, int va_level, CB cb_entry) {
  va_start = align<PAGE_SIZE, false>(va_start);
  va_end = align<PAGE_SIZE>(va_end);
  while (va_start % ENTRY_SIZE[va_level] != 0 or
         va_end % ENTRY_SIZE[va_level] != 0)
    va_level++;
  for (auto it = va_start; it < va_end; it += ENTRY_SIZE[va_level]) {
    auto& entry = walk(start, level, it, va_level);
    cb_entry(entry, it, va_level);
  }
}

void PageTable::traverse(uint64_t start, int level, CB cb_entry, CB cb_table) {
  auto nxt_start = start;
  auto nxt_level = level + 1;
  for (uint64_t idx = 0; idx < TABLE_SIZE_4K; idx++) {
    auto& entry = entries[idx];
    switch (entry.type) {
      case PD_TABLE:
        cb_table(entry, nxt_start, nxt_level);
        break;
      case PD_BLOCK:
      case PD_ENTRY:
        cb_entry(entry, nxt_start, level);
        break;
    }
    nxt_start += ENTRY_SIZE[level];
  }
}

// TODO: remove -mno-unaligned-access
void map_kernel_as_normal(uint64_t kernel_start, uint64_t kernel_end) {
  PageTableEntry PMD_entry{
      .PXN = false,
      .output_address = 0,
      .RDONLY = false,
      .KERNEL = true,
      .AttrIdx = MAIR_IDX_DEVICE_nGnRnE,
      .type = PD_BLOCK,
  };
  auto PMD = new PageTable(PMD_entry, PMD_LEVEL);

  PMD->walk(
      KERNEL_SPACE, PMD_LEVEL, mm_page.start(), mm_page.end(), PMD_LEVEL,
      [](auto& entry, auto, auto) { entry.AttrIdx = MAIR_NORMAL_NOCACHE; });
  PMD->walk(KERNEL_SPACE, PMD_LEVEL, kernel_start, kernel_end, PMD_LEVEL,
            [](auto& entry, auto, auto) { entry.RDONLY = true; });
  PMD->traverse([](auto& entry, auto, auto) { entry.PXN = true; });

  auto PUD = (PageTable*)__upper_PUD;
  PUD->entries[0].set_table(PMD);
  PUD->entries[1].PXN = true;
}
