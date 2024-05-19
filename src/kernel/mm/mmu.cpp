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

PageTable::PageTable(PageTableEntry entry, uint64_t entry_size) {
  if (entry_size == PTE_ENTRY_SIZE and entry.type == PD_BLOCK)
    entry.type = PD_ENTRY;
  uint64_t offset = entry_size / PAGE_SIZE;
  for (uint64_t i = 0; i < TABLE_SIZE_4K; i++) {
    entries[i] = entry;
    entry.output_address += offset;
  }
}

PageTableEntry& PageTable::walk(char* table_start, uint64_t entry_size,
                                char* addr, uint64_t size) {
  uint64_t idx = (addr - table_start) / entry_size;
  auto& entry = entries[idx];
  if (entry_size == size) {
    return entry;
  }
  auto nxt_table_start = table_start + idx * entry_size;
  auto nxt_entry_size = entry_size / TABLE_SIZE_4K;
  PageTable* nxt_table = new PageTable(entry, nxt_entry_size);
  entry.set_table(nxt_table);
  return nxt_table->walk(nxt_table_start, nxt_entry_size, addr, size);
}

void PageTable::walk(char* table_start, uint64_t entry_size, char* start,
                     char* end, CB callback) {
  start = align<PAGE_SIZE, false>(start);
  end = align<PAGE_SIZE>(end);
  uint64_t size = entry_size;
  while ((uint64_t)start % size != 0 or (uint64_t) end % size != 0)
    size /= TABLE_SIZE_4K;
  for (auto it = start; it < end; it += size) {
    auto& entry =
        walk((char*)KERNEL_SPACE, PMD_ENTRY_SIZE, (char*)it, entry_size);
    callback(entry);
  }
}

void PageTable::walk(CB callback) {
  for (auto& entry : entries) {
    switch (entry.type) {
      case PD_TABLE:
        entry.table()->walk(callback);
        break;
      case PD_BLOCK:
      case PD_ENTRY:
        callback(entry);
        break;
    }
  }
}

// TODO: remove -mno-unaligned-access
void map_kernel_as_normal(void* kernel_start, void* kernel_end) {
  PageTableEntry PMD_entry{
      .PXN = false,
      .output_address = 0,
      .RDONLY = false,
      .KERNEL = true,
      .AttrIdx = MAIR_IDX_DEVICE_nGnRnE,
      .type = PD_BLOCK,
  };
  auto PMD = new PageTable(PMD_entry, PMD_ENTRY_SIZE);

  PMD->walk(0, PMD_ENTRY_SIZE, (char*)mm_page.start(), (char*)mm_page.end(),
            [](auto& entry) { entry.AttrIdx = MAIR_NORMAL_NOCACHE; });
  PMD->walk(0, PMD_ENTRY_SIZE, (char*)kernel_start, (char*)kernel_end,
            [](auto& entry) { entry.RDONLY = true; });
  PMD->walk([](auto& entry) { entry.PXN = true; });

  auto PUD = (PageTable*)__upper_PUD;
  PUD->entries[0].set_table(PMD);
  PUD->entries[1].PXN = true;
}
