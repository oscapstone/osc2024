#define MAX_ORDER 7
#define PAGE_SIZE 4096  // Assuming a page size of 4KB
#define MEMORY_START 0x10000000
#define MEMORY_SIZE 0x10000000  // Managing 256MB of memory
#define FRAME_COUNT (MEMORY_SIZE / PAGE_SIZE)
#define MIN_BLOCK_SIZE PAGE_SIZE  // Minimum block size is the size of one page

typedef struct frame {
    int index;
    int start;
    int order;
    int status;
} frame_t;

void frames_init();
void merge_all(unsigned long print);
void* allocate_page(unsigned long size);
void convert_val_and_print(int len);
void demo_page_alloc();

typedef struct memory_pool {
    unsigned long start;   // Starting address of the pool
    int bitmap[PAGE_SIZE/16]; // Bitmap for free/allocated slots
    int slot_size;         // Size of each slot in bytes
    int total_slots;       // Total slots in the pool
} memory_pool_t;
void init_memory();
void * malloc(unsigned long size);
void free(void* ptr);