#include "mm/mmu.hpp"

#include "io.hpp"
#include "mm/page.hpp"
#include "sched.hpp"

void PT_Entry::print(int level) const {
  kprintf("0x%08lx Attr%d %s %s %s", (uint64_t)addr(), AttrIdx,
          PT_levelstr(level), kindstr(), apstr());
  if (UXN)
    kprintf(" UXN");
  if (PXN)
    kprintf(" PXN");
  if (AF)
    kprintf(" AF");
  kprintf("\n");
}

void PT_Entry::alloc(int level, bool kernel) {
  set_level(level);
  set_addr(kmalloc(PAGE_SIZE), isPTE() ? PTE_ENTRY : PD_BLOCK);
  if (kernel)
    UXN = true;
  else
    PXN = true;
}

PT_Entry PT_Entry::copy(int level) const {
  auto new_entry = *this;
  switch (kind()) {
    case EntryKind::TABLE:
      new_entry.set_table(level, pt_copy(table()));
      break;
    case EntryKind::ENTRY:
      // TODO: copy on write
      panic("copy entry not implemented");
      break;
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
  this->traverse([](auto, auto entry, auto, auto) { kfree(entry.addr()); },
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
  PT* nxt_table;
  if (entry.isTable()) {
    nxt_table = entry.table();
  } else {
    nxt_table = new PT(entry, nxt_level);
    entry.set_table(level, nxt_table);
  }
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

void PT::traverse(uint64_t start, int level, CB cb_entry, CB cb_table,
                  void* context) {
  auto nxt_start = start;
  auto nxt_level = level + 1;
  for (uint64_t idx = 0; idx < TABLE_SIZE_4K; idx++) {
    auto& entry = entries[idx];
    if (entry.isTable()) {
      if (cb_table) {
        cb_table(context, entry, nxt_start, nxt_level);
      } else {
        entry.table()->traverse(nxt_start, nxt_level, cb_entry, cb_table,
                                context);
      }
    } else if (entry.isEntry()) {
      cb_entry(context, entry, nxt_start, level);
    }
    nxt_start += ENTRY_SIZE[level];
  }
}

void PT::print(const char* name, uint64_t start, int level) {
  kprintf("===== %s ===== @ %p\n", name, va2pa(this));
  traverse(start, level, [](auto, auto entry, auto start, auto level) {
    kprintf("%016lx ~ %016lx -> ", start, start + ENTRY_SIZE[level]);
    entry.print();
  });
  kprintf("----------------------\n");
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

  reload_tlb();
}
