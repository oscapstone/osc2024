extern char _heap_top;//from linker script,which is the start of heap block
extern char _stack_top;//from linker script,which is the start of stack block
extern char* heap_ptr;//the heap top now

void* simple_malloc(unsigned int size);
void* get_sp();
char* mem_alin(char* ptr,int alin);
int memory_reserve(unsigned long long start,unsigned long long end);
struct frame_entry* get_block(int power);
struct frame_entry* new_frame_entry();
struct frame_entry* pop_entry(int power);
void* fr_malloc(unsigned int size);
void output_block_list();
void output_frame_array();
int buddy_init();
struct frame_entry* merge_entry(struct frame_entry* block);
struct frame_entry* invalid_entry_collector_recv(struct frame_entry* entry);
struct frame_entry* invalid_entry_collector_iter(struct frame_entry* head);
void* dy_malloc(unsigned int size);
int dy_free(unsigned long address);
unsigned long dy_frame_allo(unsigned int size,unsigned long frame_address);

struct frame_entry{
    unsigned long address;
    unsigned int power; //total size=4Kb*2^power
    unsigned int magic; //should be magic_value defined below
    struct frame_entry* next;
};

struct dy_frame_head{ //size:85byte <6*16byte
    char byte_16[64];
    char byte_64[16];
    char byte_256[4];
    char byte_1024[1];
    int counter;
};

struct block_1024{
    char padding[1024];
};
struct block_256{
    char padding[256];
};
struct block_64{
    char padding[64];
};
struct block_16{
    char padding[16];
};

struct dy_frame_map{
    struct block_16 b_16[64];
    struct block_64 b_64[16];
    struct block_256 b_256[4];
    struct block_1024 b_1024[1];
};

#define Buddy -100
#define Allocated -200
#define sentinel 0x12345678
#define mem_space_size 0x3b400000
#define frame_element_size 0x1000
#define magic_value 0x87654321