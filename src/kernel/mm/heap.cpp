#include "mm/heap.hpp"

#include <new.hpp>

#include "ds/list.hpp"
#include "int/interrupt.hpp"
#include "mm/log.hpp"
#include "mm/page.hpp"

#define MM_TYPE "heap"

inline int chunk_idx(uint64_t size) {
  if (size <= 0x10)
    return 0;
  if (size <= 0x30)
    return 1;
  if (size <= 0x50)
    return 2;
  if (size <= 0xf0)
    return 3;
  if (size <= 0x110)
    return 4;
  if (size <= 0x330)
    return 5;
  if (size <= 0x550)
    return 6;
  return -1;
}

static_assert(max_chunk_size < PAGE_SIZE);

struct FreeChunk : ListItem {};

static_assert(sizeof(FreeChunk) <= chunk_size[0]);

struct Info;

struct PageHeader {
  int idx;
  uint32_t size;
  PageHeader* next;
  char data[];
  PageHeader(int idx_, PageHeader* next_)
      : idx(idx_), size(chunk_size[idx_]), next(next_) {}
};

static_assert(sizeof(PageHeader) == 0x10);

struct Info {
  int idx;
  PageHeader* pages;
  ListHead<FreeChunk> list;
  Info(int idx_ = -1) : idx(idx_), pages(nullptr), list() {}

  void split_page(PageHeader* hdr) {
    auto sz = chunk_size[idx];
    for (uint32_t off = PAGE_SIZE; off >= sizeof(PageHeader) + sz; off -= sz)
      free(&hdr->data[off - sz]);
  }

  void alloc_page() {
    auto page = mm_page.alloc(PAGE_SIZE);
    MM_DEBUG("alloc_page = %p\n", page);
    auto hdr = new (page) PageHeader(idx, pages);
    split_page(hdr);
    hdr = pages;
  }

  void* alloc() {
    if (list.empty())
      alloc_page();
    return list.pop_front();
  }

  void free(void* ptr) {
    auto chk = new (ptr) FreeChunk;
    list.insert_front(chk);
  }

  void print() {
    kprintf("%d: size 0x%x\n", idx, chunk_size[idx]);
    kprintf("  pages: ");
    for (auto it = pages; it; it = it->next)
      kprintf("%p -> ", it);
    kprintf("\n");
#if MM_LOG_LEVEL >= 3
    kprintf("  free chk (%d): ", list.size());
    for (auto chk : list)
      kprintf("%p -> ", chk);
    kprintf("\n");
#else
    kprintf("  free chk: %d\n", list.size());
#endif
  }
};

Info info[chunk_class];

void heap_info() {
  kprintf("== Heap ==\n");
  for (int i = 0; i < chunk_class; i++)
    info[i].print();
  kprintf("---------------\n");
}

void heap_init() {
  for (int i = 0; i < chunk_class; i++)
    new (&info[i]) Info(i);
}

void* heap_malloc(uint64_t req_size) {
  save_DAIF_disable_interrupt();

  auto idx = chunk_idx(req_size);
  // MM_DEBUG("alloc 0x%lx -> 0x%x\n", req_size, chunk_size[idx]);
  auto ptr = info[idx].alloc();

  restore_DAIF();

  return ptr;
}

void heap_free(void* ptr) {
  save_DAIF_disable_interrupt();

  auto hdr = (PageHeader*)getPage(ptr);
  info[hdr->idx].free(ptr);

  restore_DAIF();
}
