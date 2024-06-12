#include "mem.h"

#include "devtree.h"
#include "mbox.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

extern char *__bss_end;
extern void *DTB_BASE;
extern void *DTB_END;
extern void *initrd_start;
extern void *initrd_end;

unsigned int BOARD_REVISION;
unsigned int BASE_MEMORY;
unsigned int NUM_PAGES;

static struct page *pageTable;
static struct page *freeList[BUDDY_MAX_ORDER + 1];
static struct object *objectCache[CACHE_MAX_ORDER + 1];

// `__bss_end` is defined in linker script
extern char *__bss_end;
static char *heap_top;

static void *simple_malloc(int size) {
  void *p = (void *)heap_top;
  if (size < 0) return 0;
  heap_top += size;
  return p;
}

void init_mem() {
  // Simple malloc init: Set heap base address
  heap_top = (char *)&__bss_end;

  if (BOARD_REVISION == 0) {
    // Get board revision
    get_board_revision();
    BOARD_REVISION = mbox[5];
  }
  // Get ARM memory base address and size
  if (NUM_PAGES == 0) {
    get_arm_memory_status();
    BASE_MEMORY = mbox[5];
    NUM_PAGES = (mbox[6] - BASE_MEMORY) / PAGE_SIZE;
  }
  // Initialize the buddy allocator
  pageTable = simple_malloc(sizeof(struct page) * NUM_PAGES);
  unsigned int current_order = BUDDY_MAX_ORDER;
  // for (int i = NUM_PAGES - 1; i >= 0; i--) {
  for (int i = 0; i < NUM_PAGES; i++) {
    pageTable[i].order = 0;
    pageTable[i].used = 0;
    pageTable[i].cache_order = -1;
    pageTable[i].prev = 0;
    pageTable[i].next = 0;
    if (i % (1 << current_order) == 0) {
      while (current_order > 0 && NUM_PAGES - i < (1 << current_order)) {
        current_order--;
      }
      pushPageToFreeList(&freeList[current_order], &pageTable[i],
                         current_order);
    }
  }
  unsigned int sa = NUM_PAGES * sizeof(struct page);

  // Reserve memory:
  // Spin tables for multicore boot
  reserveMemory(0x0, 0x1000);
  // User program
  reserveMemory(0x40000, 0x40000 + PAGE_SIZE);
  // Kernel & stack; Simple allocator
  reserveMemory(0x80000 - DEFAULT_STACK_SIZE, (unsigned long)&__bss_end + sa);
  // Initramfs
  reserveMemory((uint64_t)initrd_start, (uint64_t)initrd_end);
  // Devicetree
  reserveMemory((uint64_t)DTB_BASE, (uint64_t)DTB_END);
}

void reserveMemory(uint64_t resv_start, uint64_t resv_end) {
  // Round the start and end addresses to the page boundary
  resv_start = resv_start & ~(PAGE_SIZE - 1);
  resv_end = (resv_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  resv_end--;

  uart_puts("[INFO] Reserving memory: ");
  uart_hex(resv_start);
  uart_putc('-');
  uart_hex(resv_end);
  uart_putc(NEWLINE);

  for (int order = BUDDY_MAX_ORDER; order >= 0; order--) {
    struct page *current = freeList[order];
    while (current != 0) {
      struct page *next = current->next;
      uint64_t page_start = (current - pageTable) * PAGE_SIZE;
      uint64_t page_end = page_start + (PAGE_SIZE << order) - 1;
      if (page_start >= resv_start && page_end <= resv_end) {
        // [page <resv---resv> page]
        // Remove the page from the free list
        current->used = 1;
        removePageFromFreeList(&freeList[order], current);
      } else if (resv_start > page_end || resv_end < page_start) {
        // ---resv> [page or page] <resv---
      } else {
        // [page---resv> or <resv---page]
        // Split the page for lower order
        struct page *half = lookupBuddy(current, order - 1);
        removePageFromFreeList(&freeList[order], current);
        pushPageToFreeList(&freeList[order - 1], half, order - 1);
        pushPageToFreeList(&freeList[order - 1], current, order - 1);
      }
      current = next;
    }
  }
}

/* Buddy Allocator */

void pushPageToFreeList(struct page **list_head, struct page *page,
                        unsigned int order) {
  page->order = order;
  page->used = 0;
  page->prev = 0;
  page->next = 0;

  if (*list_head == 0 || (*list_head) < page) {
    if (*list_head != 0) (*list_head)->prev = page;
    page->next = *list_head;
    *list_head = page;
    return;
  }

  struct page *current = *list_head;
  while (current->next != 0 && page < current->next) {
    current = current->next;
  }
  page->prev = current;
  page->next = current->next;
  if (current->next != 0) current->next->prev = page;
  current->next = page;
}

struct page *popFreeList(struct page **list_head) {
  if (*list_head == 0) return 0;

  struct page *page = *list_head;
  *list_head = page->next;
  page->used = 1;
  return page;
}

void removePageFromFreeList(struct page **list_head, struct page *page) {
  if (page->prev != 0) page->prev->next = page->next;
  if (page->next != 0) page->next->prev = page->prev;
  if (page == *list_head) *list_head = page->next;
}

void printFreeListByOrder(unsigned int order) {
  struct page *page = freeList[order];
  if (page > 0) uart_puts("[BUDD]");
  while (page != 0) {
    uart_putc(TAB);
    uart_hex((unsigned int)(page - pageTable) * PAGE_SIZE);
    uart_puts("-");
    uart_hex((unsigned int)(page - pageTable + (1 << order)) * PAGE_SIZE - 1);
    uart_puts(" [");
    if (order < 10) uart_putc(' ');
    uart_dec(order);
    uart_puts("] ");
    uart_putc(NEWLINE);
    page = page->next;
  }
}

struct page *lookupBuddy(struct page *page, unsigned int order) {
  unsigned int buddy_pfn = (unsigned int)(page - pageTable) ^ (1 << order);
  return &pageTable[buddy_pfn];
}

struct page *allocatePagesByOrder(unsigned int order, int silent) {
  if (!silent) {
    uart_puts("[BUDD] ");
    uart_dec(1 << order);
    uart_puts("-page memory block requested.");
    uart_putc(NEWLINE);
  }

  for (int i = order; i <= BUDDY_MAX_ORDER; i++) {
    if (freeList[i] == 0)  // No free page available
      continue;            // Try next order
    struct page *page = popFreeList(&freeList[i]);
    page->order = order;  // Update order of the page

    while (i > order) {  // requires splitting
      i--;
      struct page *buddy = lookupBuddy(page, i);
      pushPageToFreeList(&freeList[i], buddy, i);

      if (silent) continue;
      // Print information
      unsigned int pfn = page - pageTable;
      unsigned int buddy_pfn = buddy - pageTable;
      uart_puts("[BUDD] Split ");
      uart_hex(pfn * PAGE_SIZE);
      uart_puts("-");
      uart_hex((pfn + (1 << i)) * PAGE_SIZE - 1);
      uart_puts("//");
      uart_hex(buddy_pfn * PAGE_SIZE);
      uart_puts("-");
      uart_hex((buddy_pfn + (1 << i)) * PAGE_SIZE - 1);
      uart_puts(" => [");
      uart_dec(i);
      uart_puts("]");
      uart_putc(NEWLINE);
    }
    if (!silent) {
      uart_puts("[BUDD] Memory allocated: ");
      uart_hex((unsigned int)(page - pageTable) * PAGE_SIZE);
      uart_puts("-");
      uart_hex((unsigned int)(page - pageTable + (1 << order)) * PAGE_SIZE - 1);
      uart_putc(NEWLINE);
    }

    return page;
  }
  return 0;
}

void freePages(struct page *page, unsigned int order, int silence) {
  if (!silence) {
    uart_puts("[BUDD] Free ");
    uart_dec(1 << order);
    uart_puts("-page memory block starting from ");
    uart_hex((unsigned int)(page - pageTable) * PAGE_SIZE);
    uart_putc(NEWLINE);
  }
  mergePages(page, order, silence);
}

void mergePages(struct page *page, unsigned int order, int silence) {
  struct page *current = page;
  while (order < BUDDY_MAX_ORDER) {
    struct page *buddy = lookupBuddy(current, order);
    if (buddy->order != order || buddy->used == 1) break;

    removePageFromFreeList(&freeList[order], buddy);

    if (current > buddy) {
      struct page *tmp = current;
      current = buddy;
      buddy = tmp;
    }

    order++;
    if (silence) continue;
    unsigned int pfn = current - pageTable;
    unsigned int buddy_pfn = buddy - pageTable;
    uart_puts("[BUDD] Merge ");
    uart_hex(pfn * PAGE_SIZE);
    uart_puts("-");
    uart_hex((pfn + (1 << (order - 1))) * PAGE_SIZE - 1);
    uart_puts("~~");
    uart_hex(buddy_pfn * PAGE_SIZE);
    uart_puts("-");
    uart_hex((buddy_pfn + (1 << (order - 1))) * PAGE_SIZE - 1);
    uart_puts(" => [");
    uart_dec(order);
    uart_puts("]");
    uart_putc(NEWLINE);
  }
  pushPageToFreeList(&freeList[order], current, order);
}

/* Cache Allocator */

void pushObjectToList(struct object **list_head, struct object *object,
                      unsigned int order) {
  object->order = order;
  object->next = *list_head;
  *list_head = object;
}

struct object *popObjectFromCache(struct object **list_head) {
  if (*list_head == 0) return 0;

  struct object *object = *list_head;
  *list_head = object->next;
  return object;
}

void *allocateCacheMemory(unsigned int order, int silent) {
  if (!silent) {
    uart_puts("[CACH] Allocating ");
    uart_dec(MIN_CACHE_SIZE << order);
    uart_puts(" bytes.");
    uart_putc(NEWLINE);
  }

  if (objectCache[order] == 0) {
    struct page *page = allocatePagesByOrder(0, silent);
    page->cache_order = order;
    unsigned int page_addr = (page - pageTable) * PAGE_SIZE;
    unsigned int cache_size = MIN_CACHE_SIZE << order;
    for (int i = 0; i < PAGE_SIZE; i += cache_size) {
      struct object *obj = (struct object *)(uintptr_t)(page_addr + i);
      pushObjectToList(&objectCache[order], obj, order);
    }
  }
  void *p = popObjectFromCache(&objectCache[order]);
  if (!silent) {
    uart_puts("[CACH] Allocated memory starting from ");
    uart_hex((uintptr_t)p);
    uart_putc(NEWLINE);
  }
  return p;
}

void freeCacheEntry(void *ptr, unsigned int index, int silence) {
  if (!silence) {
    uart_puts("[CACH] Free memory cache of ");
    uart_dec(MIN_CACHE_SIZE << index);
    uart_puts(" bytes starting from ");
    uart_hex((uintptr_t)ptr);
    uart_putc(NEWLINE);
  }
  pushObjectToList(&objectCache[index], ptr, index);
}

/* Dynamic Memory Allocator */

void *kmalloc(unsigned int size, int silent) {
  if (size == 0) return 0;

  if (!silent) {
    uart_puts("[INFO] Memory requested: ");
    uart_dec(size);
    uart_puts(" byte(s).");
    uart_putc(NEWLINE);
  }

  if (size > PAGE_SIZE / 2) {
    // Buddy Allocator
    int order = 0;
    while ((PAGE_SIZE << order) < size) order++;
    struct page *page = allocatePagesByOrder(order, silent);
    return (void *)((page - pageTable) * PAGE_SIZE);
  } else {
    // Cache Allocator
    int power = 0;
    while ((1 << power) < size) power++;
    int order = (power > 5) ? power - 5 : 0;
    return allocateCacheMemory(order, silent);
  }
}

void kfree(void *ptr, int silence) {
  // Check if the pointer is page-aligned
  struct page *page = &pageTable[(uintptr_t)ptr / PAGE_SIZE];
  if ((uintptr_t)ptr % PAGE_SIZE == 0) {
    // Check if the page is allocated by the buddy allocator
    if (page->cache_order == -1) {
      // Free the page using the buddy allocator
      freePages(page, page->order, silence);
      return;
    }
  }
  // Free the object using the cache allocator
  struct object *object = ptr;
  freeCacheEntry(object, page->cache_order, silence);
}
