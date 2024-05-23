#ifndef _ALLOC_H_
#define _ALLOC_H_

#define PAGE_BASE (void*)0x0
#define PAGE_END  (void*)0x3b400000
#define PAGE_SIZE 0x1000 // 4KB

#define MAX_ORDER 11
#define MAX_CHUNK 5

#define BUDDY -1     // buddy memory pages
#define ALLOCATED -2 // allocated memory pages
#define RESERVED -3 // reserved memory pages

extern int debug;

typedef struct page {
    unsigned char* addr;
    unsigned long long idx;
    int val;
    int order;
    struct page* next;
    struct page* prev;
} page;

typedef struct free_list_t {
    page* head;
    int cnt;
} free_list_t;

typedef struct page_info {
    int idx;
    struct page_info* next;
} page_info_t;

typedef struct chunk {
    unsigned char* addr;
    struct chunk* next;
} chunk;

typedef struct chunk_info {
    int idx; // chunk index
    int size; // chunk size
    int cnt; // free chunk count
    page_info_t* page_head; // page list
    chunk* chunk_head; // free chunk list
} chunk_info;

int buddy(int idx);

// print information
void page_info(page* p);
void print_chunk_info();
void free_list_info();
void check_free_list();

// helper functions
unsigned long long align_page(unsigned long long size);
int log2(unsigned long long size);
int is_page(void* addr);
int size2chunkidx(unsigned long long size);

void init_page_arr();
void init_chunk_info();
void init_page_allocator();

// related to page
void release(page* r_page);
void merge(page* m_page);
page* truncate(page* t_page, int order);


// related to free_list
void insert_page(page* new_page, int order);
page* pop_page(int order);
void erase_page(page* e_page, int order);

// related to chunk
void* chunk_alloc(int idx);
void* chunk_free(void* addr);

void alloc_init(); // main function

// APIs
void memory_reserve(void* start, void* end);
void* page_alloc(unsigned long long size);
void  page_free(void* addr);
void* kmalloc(unsigned long long size);
void  kfree(void* addr);

void *simple_malloc(unsigned long long size);

#endif