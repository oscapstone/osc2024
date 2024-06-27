#include "mem.h"

#include "command.h"
#include "devtree.h"
#include "hardware.h"
#include "mbox.h"
#include "start.h"
#include "str.h"
#include "uart.h"
#include "utils.h"

extern char *__bss_end;
extern void *DTB_BASE;
extern void *DTB_END;
extern void *initrd_start;
extern void *initrd_end;

unsigned int BOARD_REVISION;
unsigned long BASE_MEMORY;
unsigned long TOTAL_MEMORY;
unsigned int NUM_PAGES;

static struct page *pTable;
static struct page *freeList[BUDDY_MAX_ORDER + 1];
static struct object *objectCache[CACHE_MAX_ORDER + 1];

extern char *__bss_end;  // `__bss_end` is defined in linker script

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
  uart_log(INFO, "heap_top = ");
  uart_hex((uintptr_t)heap_top);
  uart_putc(NEWLINE);
  uart_log(INFO, "Simple malloc initialized.\n");

  unsigned int __attribute__((aligned(16))) mbox[36];

  // Get board revision
  if (BOARD_REVISION == 0) {
    get_board_revision(mbox);
    BOARD_REVISION = mbox[5];
  }
  // Get ARM memory base address and size
  if (NUM_PAGES == 0) {
    get_arm_memory_status(mbox);
    BASE_MEMORY = mbox[5];
    TOTAL_MEMORY = mbox[6];
    NUM_PAGES = (TOTAL_MEMORY - BASE_MEMORY) / PAGE_SIZE;
    uart_log(INFO, "Initialized pages: ");
    uart_dec(NUM_PAGES);
    uart_putc(NEWLINE);
  }

  // cmd_info();

  // Initialize the buddy allocator
  int pTableSize = sizeof(struct page) * NUM_PAGES;
  pTable = simple_malloc(pTableSize);
  uart_log(INFO, "Page Table: ");
  uart_hex((uintptr_t)pTable);
  uart_putc(NEWLINE);
  int current_order = BUDDY_MAX_ORDER;
  for (int i = 0; i < NUM_PAGES; i++) {
    pTable[i].order = 0;
    pTable[i].used = 0;
    pTable[i].cache_order = -1;
    pTable[i].prev = 0;
    pTable[i].next = 0;
    if (i % (1 << current_order) == 0) {
      while (current_order > 0 && NUM_PAGES - i < (1 << current_order)) {
        current_order--;
      }
      pushPageToFreeList(&freeList[current_order], &pTable[i], current_order);
    }
  }

  // Reserve memory:
  // PGD table
  reserveMemory(BOOT_PGD_PAGE_FRAME, BOOT_PGD_PAGE_FRAME + PAGE_SIZE);
  // PUD table
  reserveMemory(BOOT_PUD_PAGE_FRAME, BOOT_PUD_PAGE_FRAME + PAGE_SIZE);
  // PMD table
  reserveMemory(BOOT_PMD_PAGE_FRAME, BOOT_PMD_PAGE_FRAME + PAGE_SIZE);
  // Kernel & stack; page table
  reserveMemory(0x80000 - STACK_SIZE, (uintptr_t)&__bss_end + pTableSize);
  // Initramfs
  reserveMemory((uintptr_t)initrd_start, (uintptr_t)initrd_end);
  // Devicetree
  reserveMemory((uintptr_t)DTB_BASE, (uintptr_t)DTB_END);
}

void reserveMemory(uintptr_t resv_start, uintptr_t resv_end) {
  // Round the start and end addresses to the page boundary
  resv_start = TO_VIRT(resv_start & ~(PAGE_SIZE - 1));
  resv_end = TO_VIRT((resv_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

  uart_log(INFO, "Reserving memory: ");
  uart_addr_range(resv_start, resv_end);
  uart_putc(NEWLINE);
  resv_end--;

  for (int order = BUDDY_MAX_ORDER; order >= 0; order--) {
    struct page *current = freeList[order];
    while (current != 0) {
      struct page *next = current->next;
      uint64_t page_start = page_vaddr(current);
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

uintptr_t page_vaddr(struct page *p) {
  return TO_VIRT((unsigned long)(p - pTable) * PAGE_SIZE);
}

uintptr_t page_paddr(struct page *p) {
  return TO_PHYS((unsigned long)(p - pTable) * PAGE_SIZE);
}

void printFreeListByOrder(unsigned int order) {
  struct page *page = freeList[order];
  if (page > 0) uart_log(BUDD, "");
  while (page != 0) {
    uart_putc(TAB);
    uart_addr_range(page_vaddr(page), page_vaddr(page + (1 << order)));
    uart_puts(" [");
    if (order < 10) uart_putc(' ');
    uart_dec(order);
    uart_puts("]\n");
    page = page->next;
  }
}

struct page *lookupBuddy(struct page *page, unsigned int order) {
  unsigned int buddy_pfn = (unsigned int)(page - pTable) ^ (1 << order);
  return &pTable[buddy_pfn];
}

struct page *allocatePagesByOrder(unsigned int order, int silent) {
  if (!silent) {
    uart_log(BUDD, "Memory block requested: ");
    uart_dec(1 << order);
    uart_puts(" page(s).\n");
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
      uart_log(BUDD, "Split ");
      uart_addr_range(page_vaddr(page), page_vaddr(page + (1 << i)));
      uart_puts("//");
      uart_addr_range(page_vaddr(buddy), page_vaddr(buddy + (1 << i)));
      uart_puts(" => [");
      uart_dec(i);
      uart_puts("]\n");
    }
    if (!silent) {
      uart_log(BUDD, "Memory allocated: ");
      uart_addr_range(page_vaddr(page), page_vaddr(page + (1 << order)));
      uart_putc(NEWLINE);
    }

    return page;
  }
  return 0;
}

void freePages(struct page *page, unsigned int order, int silence) {
  if (!silence) {
    uart_log(BUDD, "Free ");
    uart_dec(1 << order);
    uart_puts("-page memory block starting from ");
    uart_hex(page_vaddr(page));
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
    uart_log(BUDD, "Merge ");
    uart_addr_range(page_vaddr(current),
                    page_vaddr(current + (1 << (order - 1))));
    uart_puts("~~");
    uart_addr_range(page_vaddr(buddy), page_vaddr(buddy + (1 << (order - 1))));
    uart_puts(" => [");
    uart_dec(order);
    uart_puts("]\n");
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

struct object *popObjectFromList(struct object **list_head) {
  if (*list_head == 0) return 0;

  struct object *object = *list_head;
  *list_head = object->next;
  return object;
}

void *allocateCacheMemory(unsigned int order, int silent) {
  if (!silent) {
    uart_log(CACH, "Allocating ");
    uart_dec(MIN_CACHE_SIZE << order);
    uart_puts(" bytes.");
    uart_putc(NEWLINE);
  }

  if (objectCache[order] == 0) {
    struct page *page = allocatePagesByOrder(0, silent);
    page->cache_order = order;
    uintptr_t page_addr = page_vaddr(page);
    int cache_size = MIN_CACHE_SIZE << order;
    for (int i = 0; i < PAGE_SIZE; i += cache_size) {
      struct object *obj = (struct object *)(uintptr_t)(page_addr + i);
      pushObjectToList(&objectCache[order], obj, order);
    }
  }
  void *p = popObjectFromList(&objectCache[order]);
  if (!silent) {
    uart_log(CACH, "Allocated memory starting from ");
    uart_hex((uintptr_t)p);
    uart_putc(NEWLINE);
  }
  return p;
}

void freeCacheEntry(void *ptr, unsigned int index, int silence) {
  if (!silence) {
    uart_log(CACH, "Free memory cache of ");
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
    uart_log(INFO, "Memory requested: ");
    uart_dec(size);
    uart_puts(" byte(s).");
    uart_putc(NEWLINE);
  }

  if (size > PAGE_SIZE / 2) {
    // Buddy Allocator
    int order = 0;
    while ((PAGE_SIZE << order) < size) order++;
    struct page *page = allocatePagesByOrder(order, silent);
    return (void *)page_vaddr(page);
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
  struct page *page = &pTable[TO_PHYS(ptr) / PAGE_SIZE];
  if (TO_PHYS(ptr) % PAGE_SIZE == 0) {
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
