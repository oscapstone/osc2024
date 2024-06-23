#define POOL {4, 8, 16, 32, 64, 128, 256, 512, 1024}
#define PARTITION {4, 2, 2, 2, 2, 2, 2, 2, 2}
#define POOL_SIZE 8
// max pages allowed to be partitioned
#define MAX_PAGE 64

typedef struct Page{
    void* prefix;
    // the occupied section of each partition size in page
    int usage[8];
}Page;

typedef struct DynamicAllocator{
    int chunk_option[8];
    int partition[8];
    int allocated_page_num;
    Page allocated_page_info[MAX_PAGE];

}DynamicAllocator;


void init_dynamic_allocator();
void* malloc(unsigned long size);