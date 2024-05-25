#include "alloc.h"
#include "mini_uart.h"
#include "fdt.h"
#include "initrd.h"
#include "exception.h"
#include <stdint.h>

int debug = 1;

// `bss_end` is defined in linker script
extern int __heap_top;
volatile char *heap_top;

page* page_arr = 0;
uint64_t total_page = 0;
free_list_t free_list[MAX_ORDER + 1];
chunk_info chunk_info_arr[MAX_CHUNK + 1];

int buddy(int idx) {
	return idx ^ (1 << page_arr[idx].order);
}

void page_info_addr(void* addr) {
	unsigned int idx = ((unsigned long long)addr - (unsigned long long)PAGE_BASE) / PAGE_SIZE;
	page_info(&page_arr[idx]);
}

void page_info(page* p) {
	uart_send_string("(addr: 0x");
	uart_hex((unsigned long long)p->addr);
	uart_send_string(", idx: ");
	uart_hex(p->idx);
	uart_send_string(", val: ");
	uart_hex(p->val);
	uart_send_string(", order: ");
	uart_hex(p->order);
	uart_send_string(")");
}

void print_chunk_info() {
	for(int i=0;i<MAX_CHUNK + 1;i++) {
		uart_send_string("chunk_info_arr[");
		uart_hex(i);
		uart_send_string("].cnt: ");
		uart_hex(chunk_info_arr[i].cnt);
		uart_send_string("\n");
	}
}


void free_list_info() {
	for (int i = 0; i < MAX_ORDER + 1; i++) {
		uart_send_string("free_list[");
		uart_hex(i);
		uart_send_string("].cnt: ");
		uart_hex(free_list[i].cnt);
		uart_send_string("\n");
	}
}

void check_free_list() {
	for(int i=0;i<MAX_ORDER;i++){
		page* p = free_list[i].head;
		int cnt = 0;
		while(p) {
			if(p->val != i) {
				uart_send_string("Error: ");
				page_info(p);
				uart_send_string(" is in free_list[");
				uart_hex(i);
				uart_send_string("]\n");
			}
			cnt ++;
			p = p->next;
		}
		if(cnt != free_list[i].cnt) {
			uart_send_string("Error: free_list[");
			uart_hex(i);
			uart_send_string("].cnt is ");
			uart_hex(free_list[i].cnt);
			uart_send_string(" but there are ");
			uart_hex(cnt);
			uart_send_string(" pages\n");
		}
	}
}

unsigned long long align_page(unsigned long long size) {
	return (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

int log2(unsigned long long size) {
	int order = 0;
	while (size > 1) {
		size >>= 1;
		order++;
	}
	return order;
}

int is_page(void* addr) {
	return (addr - PAGE_BASE) % PAGE_SIZE == 0;
}

int size2chunkidx(unsigned long long size) {
	// split to 6 size of chunk
	// 16, 32, 64, 128, 256, 512
	if (size <= 16) {
		return 0;
	}
	if (size <= 32) {
		return 1;
	}
	if (size <= 64) {
		return 2;
	}
	if (size <= 128) {
		return 3;
	}
	if (size <= 256) {
		return 4;
	}
	if(size <= 512){
		return 5;
	}
	return -1; // use buddy system instead
}

void init_page_arr() {
	total_page = ((uint64_t)PAGE_END - (uint64_t)PAGE_BASE) / (uint64_t)PAGE_SIZE;
	if(0 && debug) {
		uart_send_string("total_page: ");
		uart_hex(total_page);
		uart_send_string("\n");
	}
	page_arr = (page*)simple_malloc(total_page * sizeof(page));
	char* addr = PAGE_BASE;
	for (uint64_t i = 0; i < total_page; i++) {
		page_arr[i].addr = addr;
		page_arr[i].idx = i;
		page_arr[i].val = ALLOCATED;
		page_arr[i].order = 0;
		page_arr[i].next = 0; // NULL
		page_arr[i].prev = 0; // NULL
		addr += PAGE_SIZE;

		if(0 && debug) {
			page_info(&page_arr[i]);
			uart_send_string("\n");
		}
	}
}

void init_chunk_info() {
	for(int i=0;i<MAX_CHUNK;i++) {
		chunk_info_arr[i].idx = i;
		chunk_info_arr[i].size = 1 << (i + 4);
		chunk_info_arr[i].page_head = 0;
		chunk_info_arr[i].chunk_head = 0;
		chunk_info_arr[i].cnt = 0;
	}
}

void init_page_allocator() {
	for (int i = 0; i < MAX_ORDER + 1; i++) {
		free_list[i].head = 0; // NULL
		free_list[i].cnt = 0;
	}
	for(uint64_t i=0;i<total_page;i++) {
		if(page_arr[i].val == ALLOCATED) {
			release(&page_arr[i]);
		}
	}
}

void release(page* r_page) {
	// if the page is already free
	if(r_page -> val > 0){
		uart_send_string("Error: ");
		page_info(r_page);
		uart_send_string(" is already free\n");
		while(1);
	}
		
	if(debug) {
		uart_send_string("release ");
		page_info(r_page);
		uart_send_string("\n");
	}
	int order = r_page -> order;
	r_page->val = order;
	insert_page(r_page, order);
	merge(r_page);
}

void merge(page* m_page) {
	int a_idx = m_page->idx;
	if(page_arr[a_idx].val < 0) {
		uart_send_string("Error: ");
		page_info(&page_arr[a_idx]);
		uart_send_string(" is not free\n");
		while(1);
	}
	while(page_arr[a_idx].order + 1 < MAX_ORDER) {
		int b_idx = buddy(a_idx);
		if(buddy(b_idx) != a_idx 
			|| page_arr[a_idx].val < 0 
			|| page_arr[b_idx].val < 0) {
			break;
		}
		if(a_idx > b_idx) {
			// swap a_idx and b_idx
			int tmp = a_idx;
			a_idx = b_idx;
			b_idx = tmp;
		}
		page* a_page = &page_arr[a_idx];
		page* b_page = &page_arr[b_idx];
		if(0 && debug) {
			uart_send_string("merge ");
			page_info(a_page);
			uart_send_string(" and ");
			page_info(b_page);
			uart_send_string("\n");
		}
		if(b_page->order != a_page->order) {
			uart_send_string("Error: ");
			page_info(a_page);
			uart_send_string(" and ");
			page_info(b_page);
			uart_send_string(" have different order\n");
			while(1);
		}
		// b_page becomes a_page's buddy
		// b_page->order = a_page->order;
		
		// remove a_page and b_page from free_list
		erase_page(a_page, a_page->order);
		erase_page(b_page, b_page->order);
		b_page->val = BUDDY;

		// a_page's order increases
		a_page -> order++;
		a_page -> val++;
		// insert a_page to free_list
		insert_page(a_page, a_page->order);
	}
}

page* truncate(page* t_page, int order) {
	if(debug) {
		uart_send_string("truncate ");
		page_info(t_page);
		uart_send_string(" to order ");
		uart_hex(order);
		uart_send_string("\n");
	}
	int idx = t_page->idx;
	while(t_page->order > order) {
		t_page->order--;
		int buddy_idx = buddy(idx);
		page* buddy_page = &(page_arr[buddy_idx]);
		buddy_page->val = ALLOCATED;
		buddy_page->order = t_page->order;

		if(debug) {
			// split page into two buddies
			uart_send_string("split ");
			page_info(t_page);
			uart_send_string(" and ");
			page_info(buddy_page);
			uart_send_string("\n");
		}
		release(buddy_page);
	}

	return t_page;
}

void insert_page(page* new_page, int order) {
	if(new_page -> val < 0 || order < 0) {
		uart_send_string("Error: insert_page ");
		page_info(new_page);
		uart_send_string(" with val < 0\n");
		while(1);
	}
	new_page->val = order;
	new_page->order = order;
	new_page->next = free_list[order].head;
	if (free_list[order].head != 0) {
		free_list[order].head -> prev = new_page;
	}
	free_list[order].head = new_page;
	free_list[order].cnt++;
	return;
}

page* pop_page(int order) {
	if(free_list[order].cnt == 0) {
		uart_send_string("Error: pop_page from free_list[");
		uart_hex(order);
		uart_send_string("] with cnt = 0\n");
		while(1);
	}
	page* ret = free_list[order].head;
	free_list[order].head = ret->next;
	if (free_list[order].head -> next != 0) {
		free_list[order].head -> prev = 0;
	}
	free_list[order].cnt--;
	return ret;
}

void erase_page(page* e_page, int order) {
	if(e_page -> val < 0) {
		uart_send_string("Error: erase_page ");
		page_info(e_page);
		uart_send_string(" with val < 0\n");
		while(1);
	}
	if(e_page -> order != order) {
		uart_send_string("Error: erase_page ");
		page_info(e_page);
		uart_send_string(" with order ");
		uart_hex(e_page->order);
		uart_send_string(" but want to erase with order ");
		uart_hex(order);
		uart_send_string("\n");
		while(1);
	}
	if (e_page -> prev != 0) {
		e_page -> prev->next = e_page -> next;
	}
	else {
		free_list[order].head = e_page->next;
	}
	if (e_page->next != 0) {
		e_page->next->prev = e_page->prev;
	}
	e_page->next = 0;
	e_page->prev = 0;
	free_list[order].cnt--;
}
void* chunk_alloc(int idx) {
	int chunk_size = chunk_info_arr[idx].size;
	if(debug) {
		uart_send_string("chunk_alloc: ");
		uart_hex(idx);
		uart_send_string(" (size: ");
		uart_hex(chunk_size);
		uart_send_string(")\n");
		uart_send_string("\n");
	}
	
	
	if(chunk_info_arr[idx].chunk_head == 0) {
		if(debug) {
			// no available chunk
			uart_send_string("no available chunk\n");
		}
		// create a new page
		page_info_t* new_page = page_alloc(PAGE_SIZE);
		new_page -> idx = idx;

		// insert new_page to page list
		new_page -> next = chunk_info_arr[idx].page_head;
		chunk_info_arr[idx].page_head = new_page;

		// split page to chunk
		for(
			unsigned char* addr = new_page + PAGE_SIZE - chunk_size; 
			addr > new_page + sizeof(page_info_t); 
			addr -= chunk_size
		) {
			chunk* new_chunk = (chunk*)addr;
			new_chunk -> addr = addr;
			new_chunk -> next = chunk_info_arr[idx].chunk_head;
			chunk_info_arr[idx].chunk_head = new_chunk;
			chunk_info_arr[idx].cnt ++;
			if(debug) {
				// uart_hex(chunk_size);
				uart_send_string("new chunk: 0x");
				uart_hex((unsigned long long)addr);
				uart_send_string("\n");
				uart_hex((unsigned char*)new_page + sizeof(page_info_t));
				uart_send_string("\n");
			}
		}
	}
	if(debug) {
		// find available chunk
		uart_send_string("find available chunk\n");
		// print addr
		uart_send_string("chunk_info_arr[");
		uart_hex(idx);
		uart_send_string("].chunk_head->addr: 0x");
		uart_hex((unsigned long long)chunk_info_arr[idx].chunk_head->addr);
		uart_send_string("\n");
	}
	chunk* res_chunk = chunk_info_arr[idx].chunk_head;
	chunk_info_arr[idx].chunk_head = res_chunk->next;
	chunk_info_arr[idx].cnt --;
	return (void*)res_chunk->addr;
}

void* chunk_free(void* addr) {
	if(debug) {
		uart_send_string("Releasing chunk: 0x");
		uart_hex((unsigned long long)addr);
		uart_send_string("\n");
	}
	// convert chunk addr to page addr
	void* page_addr = (void*)((uint64_t)addr & (~(PAGE_SIZE - 1)));
	page_info_t* chunk_page = (page_info_t*)page_addr;
	int idx = chunk_page -> idx;
	chunk* new_chunk = (chunk*)addr;
	new_chunk -> addr = addr;
	// insert new_chunk to free list
	new_chunk -> next = chunk_info_arr[idx].chunk_head;
	chunk_info_arr[idx].chunk_head = new_chunk;
	chunk_info_arr[idx].cnt ++;
}

// Set heap base address
void alloc_init()
{
	heap_top = ((volatile unsigned char *)(0x10000000));
	char* old_heap_top = heap_top;
	// uart_send_string("heap_top: ");
	// uart_hex((unsigned long long)heap_top);
	// uart_send_string("\n");
	init_page_arr();
	// reserve memory
	memory_reserve((void*)0x0000, (void*)0x1000); // spin tables
	memory_reserve((void*)ramfs_base, (void*)ramfs_end); // ramfs
	// memory_reserve((void*)dtb_base, (void*)dtb_end); // dtb

	// kernel, bss, stack
	// 0x80000 = _start
	// 0x0200000 = __stack_end
	memory_reserve((void*)0x80000, (void*)0x0200000); 
	
	if(debug) {
		uart_send_string("old_heap_top: ");
		uart_hex((unsigned long)old_heap_top);
		uart_send_string("\n");
		uart_send_string("heap_top: ");
		uart_hex((unsigned long)heap_top);
		uart_send_string("\n");
	}
	// heap
	memory_reserve((void*)old_heap_top, (void*)heap_top);
	debug = 0;
	init_page_allocator();
	// debug = 1;
	check_free_list();
	// print the number of free pages for each order
	// if(debug) {
	// }
	free_list_info();
	// init chunk info
	init_chunk_info();
}

void memory_reserve(void* start, void* end) {
	if(debug) {
		uart_send_string("memory_reserve: ");
		uart_hex((unsigned long)start);
		uart_send_string(" ~ ");
		uart_hex((unsigned long)end);
		uart_send_string("\n");
	}
	 // Align start to the nearest page boundary (round down)
    uint64_t aligned_start = (uint64_t)start & ~(PAGE_SIZE - 1);
    // Align end to the nearest page boundary (round up)
    uint64_t aligned_end = ((uint64_t)end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint64_t s_idx = (aligned_start - (uint64_t)PAGE_BASE) / PAGE_SIZE;
    uint64_t e_idx = (aligned_end - (uint64_t)PAGE_BASE) / PAGE_SIZE;

    for (uint64_t i = s_idx; i < e_idx; i++) {
        page_arr[i].val = RESERVED;
    }
}

void* page_alloc(unsigned long long size) {
	int order = log2(align_page(size) / PAGE_SIZE);
	if(debug) {
		uart_send_string("Requesting ");
		uart_hex(size);
		uart_send_string(" bytes, order: ");
		uart_hex(order);
		uart_send_string("\n");
	}
	page* res_page = 0;
	for(int i=order; i<=MAX_ORDER; i++) {
		if(debug) {
			uart_send_string("Checking free_list[");
			uart_hex(i);
			uart_send_string("] = ");
			uart_hex(free_list[i].cnt);
			uart_send_string("\n");
		}
		if(free_list[i].cnt > 0) {
			res_page = pop_page(i);
			break;
		}
	}
	if(!res_page){
		if(0 && debug) {
			uart_send_string("No enough memory\n");
		}
		return 0;
	}
	res_page -> val = ALLOCATED;
	truncate(res_page, order);
	if(debug) {
		uart_send_string("Allocated ");
		page_info(res_page);
		uart_send_string("\n");
	}
	return (void*)res_page->addr;
}

void page_free(void* addr) {
	unsigned int idx = ((unsigned long long)addr - (unsigned long long)PAGE_BASE) / PAGE_SIZE;
	if(0 && debug) {
		uart_send_string("0x");
		uart_hex((unsigned long long)addr);
		uart_send_string(" -> ");
		uart_hex(idx);
		uart_send_string(" Freeing ");
		page_info(&page_arr[idx]);
		uart_send_string("\n");
	}
	release(&page_arr[idx]);
}

void* kmalloc(unsigned long long size) {
	el1_interrupt_disable();
	if(size == 0){
		el1_interrupt_enable();
		return 0;
	}
	void* addr = page_alloc(size);
	el1_interrupt_enable();
	// free_list_info();
	return addr;
	int idx = size2chunkidx(size);
	if(idx >= 0) {
		return chunk_alloc(idx);
	} else {
		return page_alloc(size);
	}
}

void kfree(void* addr) {
	if(!addr) return;
	el1_interrupt_disable();
	if(is_page(addr)) {
		// uart_send_string("page addr release\n");
		page_free(addr);
	}
	else {
		// uart_send_string("chunk addr release\n");
		chunk_free(addr);
	}
	// free_list_info();
	el1_interrupt_enable();
}

void *simple_malloc(unsigned long long size)
{
	void *p = (void *)heap_top;
	heap_top += size;
	return p;
}