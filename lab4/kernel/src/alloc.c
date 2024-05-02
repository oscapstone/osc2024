#include "alloc.h"
#include "mini_uart.h"
#include "fdt.h"
#include "set.h"
#include "cpio.h"

/*
total: [0x0, 0x3c000000]
each page is 4KB, thus 0x3c000 pages
kernel: [0x80000, 0x100000]
Spin tables for multicore boot [0x0000, 0x1000]
initramfs: determined after parsing
devicetree: determined after parsing
size for managing: starting from 0x100001
*/


extern char* _cpio_file;

struct node {
	int sz, par;
};

/*
struct nodeList {
	struct nodeList* next;
	int pos;
};

struct nodeList* node_arr[21];
// node_arr[20] for pool
*/


nodeList* node_arr[21];

char* manage_start = (char*)0x100001;
char* now = (char*)0x100001;
char* frame_sz;
int* frame_par;

char* frame_pool;

const int arr_size = 0x3c000;

void reserve(unsigned long l, unsigned long r) {

	if (frame_par == 0) {
		uart_printf ("manager have not initialized\r\n");
		return;
	}
	
	unsigned long l_frame = l >> 12;
	unsigned long r_frame = r >> 12;
	
	uart_printf ("Reserved [%d, %d]\r\n", l_frame, r_frame);
	
	for (unsigned long i = l_frame; i <= r_frame; i ++) {
		frame_par[i] = -1;
	}
}

void* simple_malloc(int num, int size) {
	if ((int)now % size != 0) now += size - (int)now % size; // align
	now += num * size;
	return (char*)(now - num * size);
}

void print_node_list() {
	for (int i = 0; i < 6; i ++) {
		uart_printf("size %d: \r\n", i);
		preOrder(node_arr[i]);
		uart_printf("\r\n");
	}
}

void print_pool() {
	uart_printf("cur pool:\r\n");
	preOrder(node_arr[20]);
	uart_printf("\r\n");
}

void add_node_list(int ind, int pos) {
	node_arr[ind] = insert(node_arr[ind], pos);
}

void remove_node_list(int ind, int pos) {
	node_arr[ind] = deleteNode(node_arr[ind], pos);
}

int pop_node_list(int ind) {
	int t = getAnyElement(node_arr[ind]);
	if (t == -1) {
		return t;
	}
	node_arr[ind] = deleteNode(node_arr[ind], t);
	return t;	
}


void manage_init(char* fdt) {
	frame_pool = simple_malloc(arr_size, sizeof(char));
	for (int i = 0; i < arr_size; i ++) {
		frame_pool[i] = 0;
	}
	frame_par = simple_malloc(arr_size, sizeof(int));
	frame_sz = simple_malloc(arr_size, sizeof(char));
	for (int i = 0; i < arr_size; i ++) {
		frame_par[i] = i;
		frame_sz[i] = 0;
	}
	for (int i = 0; i <= 20; i ++) {
		node_arr[i] = NULL;
	}
	
	reserve (0, 0x1000);
	reserve (0x80000, 0x100000);
	reserve (manage_start, now + sizeof(struct nodeList) * 10000);
	// TODO This is SHIT, might potentially cause problem
	reserve (fdt, get_fdt_end());
	reserve (_cpio_file, get_cpio_end());

	for (int i = 1; (1 << i) <= arr_size ; i ++) {
		for (int j = 0; j + (1 << i) <= arr_size; j += (1 << i)) {
			int l = j, r = j + (1 << (i - 1));
			if (frame_par[l] == l && frame_par[r] == r) {
				frame_par[r] = l;
				frame_sz[l] ++;
			}
		}
	}
	// build the initial layout

	for (int i = 0; i < arr_size; i ++) {
		if (frame_par[i] == i) {
			int sz = frame_sz[i];
			add_node_list(sz, i);
		}
	}
}

void* frame_malloc(size_t size) {
	uart_printf ("allocating size %d\r\n", size);
	int sz = 0, t = 1;
	while (t < size) {
		t <<= 1;
		sz ++;
	}
	
	int pos = 0, tsz;
	for (tsz = sz; tsz < 20; tsz ++) {
		pos = pop_node_list(tsz);
		if (pos != -1) {
			break;
		}
	}

	if (pos == -1) {
		return 0;
	}

	while (tsz > sz) {
		int pt = pos + (1 << (tsz - 1));
		frame_par[pt] = pt;
		frame_sz[pos] --;

		remove_node_list(tsz, pos);
		tsz --;
		add_node_list(tsz, pos);
		add_node_list(tsz, pt);

		uart_printf ("Divided %d with size %d\r\n", pos, tsz + 1);
	}

	if (frame_sz[pos] != sz) {
		uart_printf("Something went wrong at malloc...\r\n");
	}
	remove_node_list(sz, pos);
	frame_par[pos] = -1;
	uart_printf ("Got %d\r\n", pos);
	return (void*)(pos << 12);
}

void frame_free(char* addr) {
	uart_printf ("freeing frame %d\r\n", (int)addr >> 12);
	int p = (int)addr >> 12;
	frame_par[p] = p;
	add_node_list (frame_sz[p], p);
	while (1) {
		int sz = frame_sz[p];
		if ((p / (1 << sz)) % 2) {
			p -= (1 << sz);
		}
		
		int l = p, r = p + (1 << sz);
		if (r >= arr_size) {
			break;	
		}

		if (frame_par[l] == l && frame_par[r] == r) {
			remove_node_list(sz, l);
			remove_node_list(sz, r);
			frame_sz[l] ++;
			add_node_list(sz + 1, l);
			p = l;
			uart_printf ("Merged %d and %d with size %d\r\n", l, r, sz);
		}
		else {
			break;
		}
	}
}

void* my_malloc(size_t size) {
	if (size > 512) {
		return frame_malloc((size + 4096 - 1) / 4096);
	}
	if (node_arr[20] == NULL) {
		char* t = frame_malloc(1);
		for (char* i = t; i < t + 4096; i += 512) {
			add_node_list(20, i);
		}
	}
	int pos = pop_node_list(20);
	if (pos == -1) {
		return 0;
	}
	frame_pool[pos >> 12] ++;
	return pos;
}

void my_free(char* addr) {
	int pos = (int)addr >> 12;
	if (!frame_pool[pos]) {
		frame_free(addr);
		return;
	}
	add_node_list(20, addr);
	frame_pool[pos] --;
	if (frame_pool[pos] == 0) {
		for (int i = pos << 12; i < (pos << 12) + 4096; i += 512) {
			remove_node_list(20, i);
		}
		frame_free(pos << 12);
	}
}
