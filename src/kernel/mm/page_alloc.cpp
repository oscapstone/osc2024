#include "mm/page_alloc.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"
#include "math.hpp"
#include "mm/log.hpp"
#include "util.hpp"

static_assert(sizeof(PageAlloc::FreePage) <= PAGE_SIZE);

PageAlloc page_alloc;

void PageAlloc::info() {
  kprintf("== PageAlloc ==\n");
  bool reserved = false;
  for (uint64_t i = 0, r; i < length_; i++) {
    if (array_[i].type == FRAME_TYPE::RESERVED) {
      if (not reserved)
        r = i, reserved = true;
    } else if (array_[i].head()) {
      if (reserved) {
        kprintf("  frame %p ~ %p: %s\n", vpn2addr(r), vpn2addr(i),
                str(FRAME_TYPE::RESERVED));
        reserved = false;
      }
      kprintf("  frame %p ~ %p: %s\n", vpn2addr(i), vpn2end(i),
              str(array_[i].type));
    }
  }
  for (int8_t o = 0; o < total_order_; o++)
    if (not free_list_[o].empty()) {
      kprintf("  free_list %d: ", o);
      for (auto p : free_list_[o])
        kprintf("%p -> ", p);
      kprintf("\n");
    }
  kprintf("---------------\n");
}

void PageAlloc::preinit(uint64_t p_start, uint64_t p_end) {
  start_ = p_start, end_ = p_end;
  if (start_ % PAGE_SIZE or end_ % PAGE_SIZE) {
    klog("%s: start 0x%lx or end 0x%lx not align with PAGE_SIZE 0x%lx\n",
         __func__, start_, end_, PAGE_SIZE);
    prog_hang();
  }

  MM_INFO("page_alloc", "%s: 0x%lx ~ 0x%lx\n", __func__, start_, end_);

  length_ = (end_ - start_) / PAGE_SIZE;
  total_order_ = log2c(length_ + 1);
  MM_DEBUG("page_alloc", "%s: length = 0x%lx total_order = %x\n", __func__,
           length_, total_order_);
  array_ = new Frame[length_];
  for (uint64_t i = 0; i < length_; i++)
    array_[i] = {.type = FRAME_TYPE::ALLOCATED, .order = 0};
  free_list_ = new ListHead<FreePage>[total_order_];
}

void PageAlloc::reserve(void* p_start, void* p_end) {
  MM_INFO("page_alloc", "reserve: %p ~ %p\n", p_start, p_end);
  auto start = align<PAGE_SIZE, false>(p_start);
  auto end = align<PAGE_SIZE>(p_end);
  auto vs = addr2vpn_safe(start);
  auto ve = addr2vpn_safe(end);
  for (uint64_t i = vs; i < ve; i++)
    if (0 <= i and i < length_)
      array_[i] = {.type = FRAME_TYPE::RESERVED};
}

void PageAlloc::init() {
  log = false;
  for (uint64_t i = 0; i < length_; i++) {
    if (array_[i].allocated())
      release(vpn2addr(i));
  }
  log = true;
#if MM_LOG_LEVEL >= 3
  info();
#endif
}

void PageAlloc::release(PageAlloc::AllocatedPage apage) {
  auto vpn = addr2vpn(apage);
  auto order = array_[vpn].order;
  if (log)
    MM_INFO("page_alloc", "release: page %p order %d\n", apage, order);
  auto fpage = vpn2freepage(vpn);
  free_list_[order].insert_back(fpage);
  merge(fpage);
}

PageAlloc::AllocatedPage PageAlloc::alloc(PageAlloc::FreePage* fpage,
                                          bool head) {
  auto vpn = addr2vpn(fpage);
  auto order = array_[vpn].order;
  free_list_[order].erase(fpage);
  if (head) {
    if (log)
      MM_INFO("page_alloc", "alloc(+): page %p order %d\n", fpage, order);
    array_[vpn].type = FRAME_TYPE::ALLOCATED;
    auto apage = AllocatedPage(fpage);
    return apage;
  } else {
    array_[vpn].type = FRAME_TYPE::NOT_HEAD;
    return nullptr;
  }
}

PageAlloc::AllocatedPage PageAlloc::split(AllocatedPage apage) {
  auto vpn = addr2vpn(apage);
  auto o = --array_[vpn].order;
  auto bvpn = buddy(vpn);
  array_[bvpn] = {.type = FRAME_TYPE::ALLOCATED, .order = o};
  auto bpage = vpn2addr(bvpn);
  MM_DEBUG("page_alloc", "split: %p + %p\n", apage, bpage);
  return bpage;
}

void PageAlloc::truncate(AllocatedPage apage, int8_t order) {
  auto vpn = addr2vpn(apage);
  while (array_[vpn].order > order) {
    auto bpage = split(apage);
    release(bpage);
  }
}

void PageAlloc::merge(FreePage* apage) {
  auto avpn = addr2vpn(apage);
  while (array_[avpn].order + 1 < total_order_) {
    auto bvpn = buddy(avpn);
    if (buddy(bvpn) != avpn or not array_[avpn].free() or
        not array_[bvpn].free())
      break;
    auto bpage = vpn2freepage<true>(bvpn);
    if (avpn > bvpn) {
      std::swap(avpn, bvpn);
      std::swap(apage, bpage);
    }
    if (log)
      MM_INFO("page_alloc", "merge: %p + %p\n", apage, bpage);
    alloc(bpage, false);
    auto o = array_[avpn].order;
    free_list_[o].erase(apage);
    array_[avpn].order = ++o;
    free_list_[o].insert_back(apage);
  }
}

void* PageAlloc::alloc(uint64_t size) {
  save_DAIF_disable_interrupt();

  int8_t order = log2c(align<PAGE_SIZE>(size) / PAGE_SIZE);
  MM_DEBUG("page_alloc", "alloc: size 0x%lx -> order %d\n", size, order);
  AllocatedPage apage = nullptr;
  for (int8_t ord = order; ord < total_order_; ord++) {
    auto fpage = free_list_[ord].front();
    if (fpage == nullptr)
      continue;
    apage = alloc(fpage, true);
    truncate(apage, order);
    break;
  }
  if (apage)
    MM_DEBUG("page_alloc", "alloc: -> page %p\n", apage);
  else
    MM_DEBUG("page_alloc", "alloc: no page availiable\n");

  restore_DAIF();
  return apage;
}

void PageAlloc::free(void* ptr) {
  MM_DEBUG("page_alloc", "free: page %p\n", ptr);
  if (ptr == nullptr or not isPageAlign(ptr))
    return;

  save_DAIF_disable_interrupt();

  auto apage = AllocatedPage(ptr);
  release(apage);

  restore_DAIF();
}
