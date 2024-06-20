#include"memalloc.h"
#include"stdio.h"
#include"stddef.h"
#include"utils.h"
#include"dtb.h"
#include"cpio.h"
#include"str.h"
#include"peripherals/rpi_mmu.h"
char *heap_ptr = &_heap_top;

int Frame[mem_space_size/frame_element_size]={0};

//allocated_entry[i] is the entry address of entry witch's address is i*0x1000
//struct frame_entry* allocated_entry[mem_space_size/frame_element_size]={NULL}; 
struct frame_entry** allocated_entry;

struct frame_entry* block_lists[32]={NULL}; //block_lists[i] puts 4kb*2^i size frame
struct frame_entry* freed_entry=NULL;
struct frame_entry* dy_entry=NULL;
int buddy_initialized=0;
int dy_initialized=0;
int reserved_frame_counter=0;

int sentinel_init=0;

void* simple_malloc(unsigned int size){
    check_sentinel();
    heap_ptr+=size;
    heap_ptr=mem_alin(heap_ptr,4);

    //science kernel stack isn't fix in Lab5 and latter,flowing code isn't used
    //we use simple_malloc only in memalloc.c , space should be enough
    // if(heap_ptr > (char*)get_sp()){
    //     puts("error:heap overflow\r\n");
    //     heap_ptr-=size;
    //     return NULL;
    // }
    put32(heap_ptr,sentinel);
    return heap_ptr-size;
}

int check_sentinel(){
    if(!sentinel_init){
        sentinel_init=1;
        put32(heap_ptr,sentinel);
    }
    if(get32(heap_ptr) != sentinel){
        puts("error: sentinel wrong\r\n");
        reset(0x400);
        while(1);
        return 1;
    }
    return 0;
}

void* get_sp() {
    void *sp;
    __asm__("mov %0, sp" : "=r"(sp));
    return sp;
}

char* mem_alin(char* ptr,int alin){
    if((unsigned long long)ptr%alin)ptr+=(alin-(unsigned long long)ptr%alin);
    return ptr;
}

int memory_reserve(unsigned long long start,unsigned long long end){
    check_sentinel();
    if(start>=end)return 0;
    if(start%frame_element_size)start=mem_alin(start,frame_element_size)-frame_element_size;
    end=mem_alin(end,frame_element_size);
    if(end > mem_space_size || start > mem_space_size){
        puts("error:reserve out of range\r\n");
        return 1;
    }
    puts("reserve range:");
    put_hex(start);
    puts("~");
    put_hex(end);
    puts("\r\n");
    for(int i=(start/frame_element_size);i<(end/frame_element_size);i++){
        // puts("frame 0x");
        // put_hex(i);
        // puts(" is allocated.\r\n");
        Frame[i]=Allocated;
        reserved_frame_counter++;
    }
    puts("reserve_frame_counter:");
    put_hex(reserved_frame_counter);
    puts("\r\n");
    return;
}

int buddy_init(){
    check_sentinel();
    if(buddy_initialized)return 0;
    buddy_initialized=1;
    puts("allocated_entry size:");
    put_hex(sizeof(struct frame_entry*)*(mem_space_size/frame_element_size));
    puts("\r\n");
    allocated_entry=simple_malloc(sizeof(struct frame_entry*)*(mem_space_size/frame_element_size));

    for(int i=0;i<(mem_space_size/frame_element_size);i++){
        allocated_entry[i]=NULL;
    }

    memory_reserve(0x00,0x1000); //spin table
    memory_reserve(0x1000,0x5000); //kernel page talbe
    memory_reserve(0x80000,VIRT_TO_PHYS(&_stack_top)); //kernel
    memory_reserve(VIRT_TO_PHYS(_dtb_addr),VIRT_TO_PHYS(dtb_end())); //device tree
    memory_reserve(VIRT_TO_PHYS(cpio_addr),VIRT_TO_PHYS(cpio_end())); //cpio
    put_long_hex(cpio_addr);
    puts("\r\n");
    put_long_hex(cpio_end());
    puts("\r\n");
    // puts("frame array 0~999:");
    // for(int i=0;i<1000;i++)put_int(Frame[i]);
    // puts("\r\n");
    for(int i=0;i<mem_space_size/frame_element_size;i++){
        if(Frame[i]<0)continue;
        int max_power=1;
        for(int j=0;j<32;j++){
            if((i & (1 << j)) == 0){
                max_power*=2;
            }
            else break;
        }
        int max_free=0;
        int true_size=1;
        for(int j=0;j<max_power;j++){
            if(Frame[i+j] == 0){
                max_free++;
                if(true_size*2 == max_free)true_size*=2;
            }
            else break;
        }
        int true_power=0;
        for(int j=0;j<32;j++){
            if(true_size & (1<<j)){
                true_power=j;
                break;
            }
        }
        struct frame_entry* new_entry=new_frame_entry();
        new_entry->address=i*frame_element_size;
        new_entry->power=true_power;
        
        //insert entry to block_lists
        // if(block_lists[new_entry->power] == NULL){
        //     block_lists[new_entry->power]=new_entry;
        // }
        // else{
        //     new_entry->next=block_lists[new_entry->power];
        //     block_lists[new_entry->power]=new_entry;
        // }
        new_entry->next=block_lists[new_entry->power];
        block_lists[new_entry->power]=new_entry;

        Frame[i]=true_power;
        for(int j=1;j<true_size;j++){
            Frame[i+j]=Buddy;
        }
        i=i+true_size-1;
    }
    // output_frame_array();
    // puts("\r\n");
    // output_block_list();
    return 0;
}

struct frame_entry* new_frame_entry(){
    check_sentinel();
    static int num_of_entry_counter=0;
    struct frame_entry* entry=NULL;
    if(num_of_entry_counter >= 0x3b400){
        puts("memory usage is high\r\n");
        invalid_entry_collector();
    }
    if(NULL != freed_entry){
        entry=freed_entry;
        freed_entry=freed_entry->next;
    }
    else{
        if(num_of_entry_counter >= 0x3b400){
            puts("warning: num of frame entry exceed 0x3b400\r\n");
        }
        num_of_entry_counter++;
        // puts("num_of_entry_counter value:");
        // put_int(num_of_entry_counter);
        // puts("\r\n");
        entry=simple_malloc(sizeof(struct frame_entry));
        // puts("simple malloc address:");
        // put_hex(entry);
        // puts("\r\n");
    }
    entry->address=NULL;
    entry->next=NULL;
    entry->power=0;
    entry->magic=magic_value;

    return entry;
}
void output_frame_array(){
    check_sentinel();
    for(int i=0x1;i<mem_space_size/frame_element_size;i++){
        if(Frame[i] != Buddy && Frame[i] != Allocated){
            puts("index:");
            put_hex(i*0x1000);
            puts(" value:");
            put_int(Frame[i]);
            puts("\r\n");
        }
    }

    return;
}
void output_block_list(){
    check_sentinel();
    invalid_entry_collector();
    puts("================\r\n");
    for(int i=31;i>=0;i--){
        if(block_lists[i] == NULL)continue;
        puts("power:");
        put_int(i);
        puts("\r\n");
        struct frame_entry* current=block_lists[i];
        while(current != NULL){
            puts("address:");
            put_hex(current->address);
            puts("\r\n");
            current=current->next;
        }
    }
    puts("================\r\n");
    return;
}

void* fr_malloc(unsigned int size){
    if(size <= 0){
        puts("error:size\r\n");
        return NULL;
    }
    check_sentinel();
    if(!buddy_initialized){
        buddy_init();
    }
    unsigned int temp=size,min=-1,max=-1,need_power=0;
    for(int i=0;i<32;i++){
        if(size & (1<<i)){
            if(min == -1)min=i;
            max=i;
        }
    }
    if(max > min)need_power=max+1;
    else need_power=max;
    if(need_power<12)need_power=12;// minimal frame size is 4kb=2^12kb
    int frame_power=need_power-12;
    // puts("need list index:");
    // put_int(frame_power);
    // puts("\r\n");

    struct frame_entry* allocated_frame=get_block(frame_power);
    if(allocated_frame == NULL){
        puts("error: have no physical frame\r\n");
        return NULL;
    }
    else{
        allocated_entry[allocated_frame->address/frame_element_size]=allocated_frame;
        Frame[allocated_frame->address/frame_element_size]=Allocated;
        puts("frame allocator:");
        put_hex(allocated_frame->address);
        puts("~");
        put_hex(allocated_frame->address+(1<<need_power));
        puts("\r\n");
        return PHYS_TO_VIRT(allocated_frame->address);
    }
            
     
}

struct frame_entry* get_block(int power){ //get new 4kb*2^power blocks
    while(1){
        check_sentinel();
        if(power<0){
            puts("error: power<0.\r\n");
            return NULL;
        }
        if(block_lists[power] == NULL){
            if(power == 31){
                //puts("out of memory.\r\n");
                return NULL;
            }
            struct frame_entry* sub_block1=get_block(power+1);
            if(sub_block1 == NULL){
                return NULL;
            }
            
            
            struct frame_entry* sub_block2=new_frame_entry();
            if(sub_block2 == NULL){
                puts("error:heap overflow?\r\n");
                return NULL;
            }
            sub_block2->address=sub_block1->address | (1 << (power+12));
            if(sub_block2->address == sub_block1->address){
                puts("error: uncutable\r\n");
            }
            sub_block2->power=power;
            sub_block1->power=power;
            Frame[sub_block1->address/frame_element_size]=power;
            Frame[sub_block2->address/frame_element_size]=power;
            // puts("@@@cut: ");
            // put_hex(sub_block1->address);
            // puts("~");
            // put_hex(sub_block1->address+(1 << (sub_block1->power+12+1)));
            // puts("\r\nto: ");
            // put_hex(sub_block1->address);
            // puts("~");
            // put_hex(sub_block1->address+(1 << (sub_block1->power+12)));
            // puts(" and ");
            // put_hex(sub_block2->address);
            // puts("~");
            // put_hex(sub_block2->address+(1 << (sub_block2->power+12)));
            // puts("\r\n");


            insert_entry(sub_block1);
            insert_entry(sub_block2);
        }
        struct frame_entry* temp_block=pop_entry(power);
        if(temp_block->power != Frame[temp_block->address/frame_element_size]){
            // puts("invalid block,address:0x");
            // put_hex(temp_block->address);
            // puts("\r\n");
            temp_block->address=NULL;
            temp_block->power=-1;
            temp_block->next=freed_entry;
            freed_entry=temp_block;
            continue;
        }
        else return temp_block;
    }
}

void fr_free(unsigned long address){
    address = VIRT_TO_PHYS(address);
    if(address > 0x3b400000){
        puts("error: freed block address wrong\r\n");
        return;
    }
    check_sentinel();
    struct frame_entry* freed_block=allocated_entry[address/frame_element_size];
    if(freed_block == NULL){
        puts("error: target block is free\r\n");
        return;
    }
    if(freed_block->address != address){
        puts("error: address wrong\r\n");
        return;
    }
    allocated_entry[address/frame_element_size]=NULL;
    Frame[address/frame_element_size]=freed_block->power;
    puts("frame_free:");
    put_hex(freed_block->address);
    puts("~");
    put_hex(freed_block->address+(1 << freed_block->power+12));
    puts("\r\n");
    freed_block=merge_entry(freed_block);
    insert_entry(freed_block);

    return;
}

struct frame_entry* merge_entry(struct frame_entry* block){
    check_sentinel();
    // puts("in merge_entry:\r\n");
    // puts("freed block power:");
    // put_int(block->power);
    // puts("\r\n");
    // puts("Frame[i] value:");
    // put_int(Frame[block->address/frame_element_size]);
    // puts("\r\n");
    unsigned long twin_address=NULL;
    if(block->address & (1 << (block->power+12))){
        twin_address=block->address & ~(1 << (block->power+12));
    }
    else{
        twin_address=block->address | (1 << (block->power+12));
    }
    if(twin_address == block->address){
        puts("error: twin address\r\n");
    }
    // puts("freed block address:0x");
    // put_hex(block->address);
    // puts("\r\n");
    // puts("twin block address:0x");
    // put_hex(twin_address);
    // puts("\r\n");

    if(Frame[twin_address/frame_element_size] == block->power){ //twin_block isn't allocated and size is equal to freed block
        //puts("need merge:\r\n");
        unsigned long main_address= (block->address < twin_address) ? (block->address) : (twin_address);
        unsigned long sub_address= (block->address > twin_address) ? (block->address) : (twin_address);
        // puts("@@@merge: ");
        // put_hex(main_address);
        // puts("~");
        // put_hex(main_address+(1 << (block->power+12)));
        // puts(" and ");
        // put_hex(sub_address);
        // puts("~");
        // put_hex(sub_address+(1 << (block->power+12)));
        // puts("\r\n");
        

        block->address=main_address;
        block->power+=1;
        Frame[main_address/frame_element_size]=block->power;
        Frame[sub_address/frame_element_size]=Buddy;

        return  merge_entry(block);
    }
    else {
        //puts("no merge\r\n");
        return block;

    }
}

void insert_entry(struct frame_entry* entry){
    check_sentinel();
    if(entry == NULL){
        puts("error: inserted entry is null.\r\n");
        return;
    }
    entry->next=block_lists[entry->power];
    block_lists[entry->power]=entry;

    return;
}

struct frame_entry* pop_entry(int power){
    check_sentinel();
    if(block_lists[power] == NULL){
        puts("error: on block in target list.\r\n");
        return NULL;
    }
    struct frame_entry* poped_entry=block_lists[power];
    block_lists[power]=block_lists[power]->next;
    poped_entry->next=NULL;
    return poped_entry;
}

int invalid_entry_collector(){
    check_sentinel();
    for(int i=0;i<32;i++){
        block_lists[i]=invalid_entry_collector_iter(block_lists[i]);
    }

    return 0;
}

struct frame_entry* invalid_entry_collector_recv(struct frame_entry* entry){
    check_sentinel();
    if(entry == NULL)return NULL;
    entry->next=invalid_entry_collector_recv(entry->next);
    if(Frame[entry->address/frame_element_size] != entry->power){
        struct frame_entry* next=entry->next;
        entry->address=NULL;
        entry->power=-1;
        entry->next=freed_entry;
        freed_entry=entry;

        return next;
    }
    else{
        return entry;
    }
}

struct frame_entry* invalid_entry_collector_iter(struct frame_entry* head){
    check_sentinel();
    if(head == NULL)return NULL;
    struct frame_entry* current_a=head;
    struct frame_entry* current_b=head->next;

    while(current_b != NULL){
        if(Frame[current_b->address/frame_element_size] != current_b->power){
            current_a->next=current_b->next;
            current_b->address=NULL;
            current_b->power=-1;
            current_b->next=freed_entry;
            freed_entry=current_b;

            current_b=current_a->next;

            continue;
        }
        else{
            current_a=current_a->next;
            current_b=current_b->next;
        }
    }
    if(Frame[head->address/frame_element_size] != head->power){
        struct frame_entry* next=head->next;
        head->address=NULL;
        head->power=-1;
        head->next=freed_entry;
        freed_entry=head;
        return next;
    }
    else return head;

}

void* dy_malloc(unsigned int size){
    if(!dy_initialized){
        if(dy_init()){
            puts("error: dy_init fault");
            return NULL;
        }
    }

    unsigned int allocate_size=0;
    if(size <= 16)allocate_size=16;
    else if(size <= 64)allocate_size=64;
    else if(size <= 256)allocate_size=256;
    else if(size <= 1024)allocate_size=1024;
    else{
        puts("error: dy_malloc: allocate size too big.\r\n");
        return NULL;
    }

    struct frame_entry* current=dy_entry;
    while(1){
        unsigned long block_address=dy_frame_allo(allocate_size,current->address);
        if(block_address == NULL){
            if(current->next == NULL){
                unsigned long new_frame_address=fr_malloc(frame_element_size);
                current->next=allocated_entry[new_frame_address/frame_element_size];
                dy_frame_init(new_frame_address);
            }
            current=current->next;
            continue;
        }
        else{
            return block_address;
        }
    }
}

int dy_free(unsigned long address){
    if(!dy_initialized){
        if(dy_init()){
            puts("error: dy_init fault");
            return NULL;
        }
    }
    struct frame_entry* current=dy_entry;
    while(current->address != (address & (~0xfff)) && current != NULL)current=current->next;
    if(current == NULL){
        puts("error: have no frame of target block\r\n");
        return 1;
    }
    if(dy_frame_free(current->address,address)){
        puts("error:2\r\n");
        return 1;
    }
    return 0;
}

int dy_init(){
    puts("dynamic allocator init:\r\n");
    if(sizeof(struct dy_frame_map) != 0x1000){puts("error: dy_frame_map size error\r\n");return 1;}
    if(dy_entry == NULL){
        unsigned long address=fr_malloc(frame_element_size);
        dy_entry=allocated_entry[address/frame_element_size];
        if(dy_entry->address != address){
            puts("error: dy_malloc error 1\r\n");
            return 1;
        }
        dy_frame_init(address);
    }

    dy_initialized=1;
    return 0;
}

int dy_frame_init(unsigned long address){
    if(((address & 0xfff) || (address == NULL))){
        puts("error: dy_frame_init address wrong\r\n");
        return 1;
    }
    struct dy_frame_head* head=address;
    int num_reserv_block=(sizeof(struct dy_frame_head)/16)+1;
    for(int i=0;i<64;i++){
        if(i<num_reserv_block){
            head->byte_16[i]=1; //reserve head block
            continue;
        }
        head->byte_16[i]=0;
    }
    for(int i=0;i<16;i++){
        head->byte_64[i]=0;
    }
    for(int i=0;i<16;i++){
        head->byte_64[i]=0;
    }
    for(int i=0;i<4;i++){
        head->byte_256[i]=0;
    }
    for(int i=0;i<1;i++){
        head->byte_1024[i]=0;
    }
    head->counter=85-num_reserv_block;

    //output_dy_frame_state(address);
    return 0;
}

int output_dy_frame_state(unsigned long address){
    puts("dy_frame state:\r\n");
    puts("frame address:");
    put_hex(address);
    puts("\r\n");
    char* current=address;
    struct dy_frame_head* head=address;
    for(int i=0;i<85;i++){
        put_int(*(current++));
    }
    puts("\r\ncounter:");
    put_int(head->counter);
    puts("\r\n");
}

unsigned long dy_frame_allo(unsigned int size,unsigned long frame_address){
    struct dy_frame_head* head=frame_address;
    struct dy_frame_map* map=frame_address;
    if(size == 16){
        for(int i=0;i<64;i++){
            if(head->byte_16[i] == 0){
                head->byte_16[i]=1;
                head->counter--;
                return &(map->b_16[i]);
            }
        }
        return NULL;
    }
    else if(size == 64){
        for(int i=0;i<16;i++){
            if(head->byte_64[i] == 0){
                head->byte_64[i]=1;
                head->counter--;
                return &(map->b_64[i]);
            }
        }
        return NULL;
    }
    else if(size == 256){
        for(int i=0;i<4;i++){
            if(head->byte_256[i] == 0){
                head->byte_256[i]=1;
                head->counter--;
                return &(map->b_256[i]);
            }
        }
        return NULL;
    }
    else if(size == 1024){
        for(int i=0;i<1;i++){
            if(head->byte_1024[i] == 0){
                head->byte_1024[i]=1;
                head->counter--;
                return &(map->b_1024[i]);
            }
        }
        return NULL;
    }
    else{
        puts("error: in dy_frame_allo,allocate size wrong\r\n");
        return NULL;
    }
}

int dy_frame_free(unsigned long frame_address,unsigned long block_address){
    if((block_address&(~(0xfff))) != frame_address){
        puts("error:dy_frmae_free:address wrong\r\n");
        return 1;
    }
    struct dy_frame_head* head=frame_address;
    struct dy_frame_map* map=frame_address;
    if((block_address & 0xfff) < 0x400){
        for(int i=0;i<64;i++){
            if(&(map->b_16[i]) == block_address){
                if(head->byte_16[i] != 1){
                    puts("error: dy_frame_free : blcok is free\r\n");
                    return 1;
                }
                head->byte_16[i]=0;
                head->counter++;
                return 0;
            }
        }
    }
    else if((block_address & 0xfff) < 0x800){
        for(int i=0;i<16;i++){
            if(&(map->b_64[i]) == block_address){
                if(head->byte_64[i] != 1){
                    puts("error: dy_frame_free : blcok is free\r\n");
                    return 1;
                }
                head->byte_64[i]=0;
                head->counter++;
                return 0;
            }
        }
    }
    else if((block_address & 0xfff) < 0xC00){
        for(int i=0;i<4;i++){
            if(&(map->b_256[i]) == block_address){
                if(head->byte_256[i] != 1){
                    puts("error: dy_frame_free : blcok is free\r\n");
                    return 1;
                }
                head->byte_256[i]=0;
                head->counter++;
                return 0;
            }
        }
    }
    else{
        for(int i=0;i<1;i++){
            if(&(map->b_1024[i]) == block_address){
                if(head->byte_1024[i] != 1){
                    puts("error: dy_frame_free : blcok is free\r\n");
                    return 1;
                }
                head->byte_1024[i]=0;
                head->counter++;
                return 0;
            }
        }
    }
    puts("error: dy_frmae_free: error1\r\n");
    return 1;
}