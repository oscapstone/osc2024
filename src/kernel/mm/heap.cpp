#include "mm/heap.hpp"

#include <new.hpp>

#include "ds/list.hpp"
#include "mm/new.hpp"
#include "mm/page_alloc.hpp"

constexpr uint32_t chunk_size[] = {
    0x10, 0x30, 0x50, 0xf0, 0x110, 0x330, 0x550,
};

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

constexpr int chunk_class = sizeof(chunk_size) / sizeof(uint32_t);
constexpr uint32_t max_chunk_size = chunk_size[chunk_class - 1];

static_assert(max_chunk_size <= PAGE_SIZE);

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
    auto page = page_alloc.alloc(PAGE_SIZE);
    MM_DEBUG("heap", "alloc_page = %p\n", page);
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
    kprintf("  free chk: %d\n", list.size());
    /*
    kprintf("  free: ");
    for (auto chk : list)
      kprintf("%p -> ", chk);
    kprintf("\n");
    */
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
  set_new_delete_handler(heap_malloc, heap_free);
  for (int i = 0; i < chunk_class; i++)
    new (&info[i]) Info(i);
}

// TODO: handle alignment
void* heap_malloc(uint64_t req_size, uint64_t align) {
  if (req_size > max_chunk_size)
    return page_alloc.alloc(req_size);

  auto idx = chunk_idx(req_size);

  return info[idx].alloc();
}

void heap_free(void* ptr) {
  if (isPageAlign(ptr))
    return page_alloc.free(ptr);
  auto hdr = (PageHeader*)getPage(ptr);
  info[hdr->idx].free(ptr);
}
