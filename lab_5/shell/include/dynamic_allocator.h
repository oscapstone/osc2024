#define POOL {4, 8, 16, 32, 64, 128, 256, 512, 1024}
#define PARTITION {4, 2, 2, 2, 2, 2, 2, 2, 2}
#define POOL_SIZE 9
// max pages allowed to be partitioned
#define MAX_PAGE 64

typedef struct Page{
    void* prefix;
    // the occupied section of each partition size in page, 0 mean available
    int slot[9][4];
}Page;

typedef struct DynamicAllocator{
    int allocated_page_num;
    Page allocated_page_info[MAX_PAGE];

}DynamicAllocator;


void init_dynamic_allocator();
void* malloc(unsigned long size);