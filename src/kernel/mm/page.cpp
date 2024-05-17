#include "mm/page.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"
#include "math.hpp"
#include "mm/log.hpp"
#include "util.hpp"

#define MM_TYPE "page"

static_assert(sizeof(PageSystem::FreePage) <= PAGE_SIZE);

PageSystem mm_page;

void PageSystem::info() {
  kprintf("== PageAlloc ==\n");
  bool reserved = false;
  for (uint64_t i = 0, r; i < length_; i++) {
    if (array_[i].type == FRAME_TYPE::RESERVED) {
      if (not reserved)
        r = i, reserved = true;
    } else if (array_[i].head()) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
      // flag '0' results in undefined behavior with 'p' conversion specifier
      if (reserved) {
        kprintf("  frame %010p ~ %010p: %s\n", pfn2addr(r), pfn2addr(i),
                str(FRAME_TYPE::RESERVED));
        reserved = false;
      }
      kprintf("  frame %010p ~ %010p: %s\n", pfn2addr(i), pfn2end(i),
              str(array_[i].type));
#pragma GCC diagnostic pop
    }
  }
  for (int8_t o = 0; o < total_order_; o++)
    if (not free_list_[o].empty()) {
      kprintf("  free_list %d: ", o);
      save_DAIF_disable_interrupt();
      for (auto p : free_list_[o])
        kprintf("%p -> ", p);
      restore_DAIF();
      kprintf("\n");
    }
  kprintf("---------------\n");
}

void PageSystem::preinit(uint64_t p_start, uint64_t p_end) {
  start_ = p_start, end_ = p_end;
  if (start_ % PAGE_SIZE or end_ % PAGE_SIZE)
    panic("[%s] start 0x%lx or end 0x%lx not align with PAGE_SIZE 0x%lx",
          __func__, start_, end_, PAGE_SIZE);

  MM_INFO("%s: 0x%lx ~ 0x%lx\n", __func__, start_, end_);

  length_ = (end_ - start_) / PAGE_SIZE;
  total_order_ = log2c(length_ + 1);
  MM_DEBUG("%s: length = 0x%lx total_order = %x\n", __func__, length_,
           total_order_);
  array_ = new Frame[length_];
  for (uint64_t i = 0; i < length_; i++)
    array_[i] = {.type = FRAME_TYPE::ALLOCATED, .order = 0};
  free_list_ = new ListHead<FreePage>[total_order_];
}

void PageSystem::reserve(void* p_start, void* p_end) {
  MM_INFO("reserve: %p ~ %p\n", p_start, p_end);
  auto start = align<PAGE_SIZE, false>(p_start);
  auto end = align<PAGE_SIZE>(p_end);
  auto vs = addr2pfn_safe(start);
  auto ve = addr2pfn_safe(end);
  for (uint64_t i = vs; i < ve; i++)
    if (0 <= i and i < length_)
      array_[i] = {.type = FRAME_TYPE::RESERVED};
}

void PageSystem::init() {
  log = false;
  for (uint64_t i = 0; i < length_; i++) {
    if (array_[i].allocated())
      release(pfn2addr(i));
  }
  log = true;
#if MM_LOG_LEVEL >= 3
  info();
#endif
}

void PageSystem::release(PageSystem::AllocatedPage apage) {
  auto pfn = addr2pfn(apage);
  auto order = array_[pfn].order;
  if (log)
    MM_VERBOSE("release: page %p order %d\n", apage, order);
  auto fpage = pfn2freepage(pfn);
  free_list_[order].push_back(fpage);
  merge(fpage);
}

PageSystem::AllocatedPage PageSystem::alloc(PageSystem::FreePage* fpage,
                                            bool head) {
  auto pfn = addr2pfn(fpage);
  auto order = array_[pfn].order;
  free_list_[order].erase(fpage);
  if (head) {
    if (log)
      MM_VERBOSE("alloc(+): page %p order %d\n", fpage, order);
    array_[pfn].type = FRAME_TYPE::ALLOCATED;
    auto apage = AllocatedPage(fpage);
    return apage;
  } else {
    array_[pfn].type = FRAME_TYPE::NOT_HEAD;
    return nullptr;
  }
}

PageSystem::AllocatedPage PageSystem::split(AllocatedPage apage) {
  auto pfn = addr2pfn(apage);
  auto o = --array_[pfn].order;
  auto bpfn = buddy(pfn);
  array_[bpfn] = {.type = FRAME_TYPE::ALLOCATED, .order = o};
  auto bpage = pfn2addr(bpfn);
  MM_VERBOSE("split: %p + %p\n", apage, bpage);
  return bpage;
}

void PageSystem::truncate(AllocatedPage apage, int8_t order) {
  auto pfn = addr2pfn(apage);
  while (array_[pfn].order > order) {
    auto bpage = split(apage);
    release(bpage);
  }
}

void PageSystem::merge(FreePage* apage) {
  auto apfn = addr2pfn(apage);
  while (array_[apfn].order + 1 < total_order_) {
    auto bpfn = buddy(apfn);
    if (buddy(bpfn) != apfn or not array_[apfn].free() or
        not array_[bpfn].free())
      break;
    auto bpage = pfn2freepage<true>(bpfn);
    if (apfn > bpfn) {
      std::swap(apfn, bpfn);
      std::swap(apage, bpage);
    }
    if (log)
      MM_VERBOSE("merge: %p + %p\n", apage, bpage);
    alloc(bpage, false);
    auto o = array_[apfn].order;
    free_list_[o].erase(apage);
    array_[apfn].order = ++o;
    free_list_[o].push_back(apage);
  }
}

void* PageSystem::alloc(uint64_t size) {
  int8_t order = log2c(align<PAGE_SIZE>(size) / PAGE_SIZE);
  MM_DEBUG("alloc: size 0x%lx -> order %d\n", size, order);

  save_DAIF_disable_interrupt();
  AllocatedPage apage = nullptr;
  for (int8_t ord = order; ord < total_order_; ord++) {
    auto fpage = free_list_[ord].front();
    if (fpage == nullptr)
      continue;
    apage = alloc(fpage, true);
    truncate(apage, order);
    break;
  }
  restore_DAIF();

  if (apage)
    MM_DEBUG("alloc: -> page %p\n", apage);
  else
    MM_DEBUG("alloc: no page availiable\n");

  return apage;
}

void PageSystem::free(void* ptr) {
  MM_DEBUG("free: page %p\n", ptr);
  if (ptr == nullptr or not isPageAlign(ptr))
    return;

  save_DAIF_disable_interrupt();

  auto apage = AllocatedPage(ptr);
  release(apage);

  restore_DAIF();
}
