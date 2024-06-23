#include "include/page_allocator.h"
#include "include/utils.h"
#include "include/uart.h"

struct TheArray the_array[AVAILABLE_PAGE_NUM];
OrderList order_list[MAX_ORDER];


/*Pop the top element in linked list, note I assume there EXIST AT LEAST ONE AVAILABLE NODE*/
struct Node* list_pop(OrderList* target){
    Node* to_ret = target->root;
    target->root = target->root->next;
    return to_ret;
}

void init_order_list(){
    int current_order = MAX_ORDER-1;
    int current_page_index = 0;
    // assume all pages are available at beginnig, greedly allocate pages in max order size
    while(current_order >= 0){
        while((current_page_index+_pow(2, current_order)-1) < AVAILABLE_PAGE_NUM){
            unsigned int allocated_size = _pow(2, current_order)*PAGE_SIZE;
            // record the grouped block at The Array
            the_array[current_page_index].block_size = _pow(2, current_order);
            // malloc a struct node to record the meta data of this block
            Node* new_node = simple_malloc(sizeof(Node));
            // push into order list
            new_node->address = (unsigned char*) HEAP_START_ADDRESS+(PAGE_SIZE*current_page_index);
            new_node->page_index = current_page_index;
            list_push_back(new_node, current_order);

            uart_puts("[Init] grouping block at address 0x");
            uart_hex(new_node->address);
            uart_puts(" with 0x");
            uart_hex((unsigned int)allocated_size/PAGE_SIZE);
            uart_puts(" pages and start at index ");
            uart_hex((unsigned int)new_node->page_index);
            uart_puts(" in The Array");
            uart_puts("\n");

            current_page_index = current_page_index+_pow(2, current_order)+1;
        }
        current_order--;
    }

}

void print_order_list(OrderList* list){
    Node* cur = list->root;
    uart_puts("address : \n");
    while(cur != NULL){
        uart_hex(cur->address);
        uart_puts("\n");
        cur = cur->next;
        
    }
}

void init_buddy_system(){
    // initialize The Array
    for(int i=0;i<AVAILABLE_PAGE_NUM; i++){
        the_array[i].available = FREE;
        the_array[i].block_size = 0;
    }

    // initialize order_list
    for(int i=0;i<MAX_ORDER;i++){
        order_list[i].chain_length=0;
        order_list[i].root = NULL;
    }
    init_order_list();
}


int check_list_has_element(OrderList* target){
    return target->root != NULL;
}

Node* get_buddy(OrderList* target, int target_page_index){
    Node* cur = target->root;
    Node* prev = NULL;
    while(cur != NULL){
        if(cur->page_index == target_page_index){
            if(prev != NULL){
                prev->next = cur->next;
            }
            // root is the buddy
            else{
                target->root = target->root->next;
            }
            cur->next = NULL;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    uart_puts("[Assertion Error] buddy not found\n");
    print_order_list(target);
    return NULL;
}

void free_page(void* to_free){
    int page_index = ((unsigned int)to_free-HEAP_START_ADDRESS)/PAGE_SIZE;
    int free_block_size = the_array[page_index].block_size;
    int do_merge = 0;
    if(the_array[page_index].available != OCCUPIED){
        uart_puts("[Assertion Error] releasing free page head\n");
    }
    the_array[page_index].available = FREE;
    int next_block_index = page_index+free_block_size+1;
    uart_puts("[Free] block that contains ");
    // block size starts from zero, thus need to +1
    uart_hex(free_block_size+1);
    uart_puts(" pages started from address 0x");
    uart_hex((unsigned int)to_free);
    uart_puts(" to address 0x");
    uart_hex((unsigned int)to_free+(free_block_size*PAGE_SIZE));
    uart_puts(" with index starts from ");
    uart_hex(page_index);
    uart_puts("\n");

    if(the_array[next_block_index].available == FREE){
        // debug: print the address of next page of freed block
        // uart_puts("Accessing location: 0x");
        // uart_hex((unsigned int)HEAP_START_ADDRESS+(PAGE_SIZE*next_block_index));
        // uart_puts("\n");

        // same size, can merge
        if(the_array[next_block_index].block_size == free_block_size && the_array[next_block_index].available==FREE){
            int order =0, block_size = free_block_size;
            // get sqrt(2) of block_size to obtain order
            while(block_size > 0){
                order++;
                block_size/=2;
            }
    print_order_list(&order_list[order]);
            Node* buddy = get_buddy(&order_list[order], next_block_index);
            if(buddy != NULL){
                Node* new_node = simple_malloc(sizeof(Node));
                // push into order list
                new_node->address = (unsigned char*) to_free;
                new_node->page_index = page_index;
                the_array[page_index].block_size = _pow(2, order+1);
                the_array[next_block_index].available = FREE;
                the_array[next_block_index].block_size = FREE;
                list_push_back(new_node, order+1);
                uart_puts("[Merge] block that contains ");
                uart_hex(free_block_size);
                uart_puts(" pages started from address 0x");
                uart_hex((unsigned int)to_free);
                uart_puts(" to address 0x");
                uart_hex((unsigned int)HEAP_START_ADDRESS+(PAGE_SIZE*(next_block_index+free_block_size)));
                uart_puts("\n");
                do_merge = 1;
            }
        }
    }
    if(!do_merge){
        Node* new_node = simple_malloc(sizeof(Node));
        new_node->address = (unsigned char*) to_free;
        new_node->page_index = page_index;
        int order =0, block_size = free_block_size;
        // get sqrt(2) of block_size to obtain order
        while(block_size > 0){
            order++;
            block_size/=2;
        }
        list_push_back(new_node, order);
    }
    
}

unsigned char* malloc_page(int request_page_num){
    //determine the minimum list order that this request needs
    int current_order = 0;
    int cut_freg = 0;
    // start from allocating the smallest block until is can satisfy needs
    //print_order_list(&order_list[2]);
    while(current_order<MAX_ORDER){
        // TODO: Mysterious print, without it the mallocation will failed on resp pi 3B+
        uart_puts("");
        if(request_page_num > _pow(2, current_order)){
            current_order++;
        }
        else{
            if(check_list_has_element(&order_list[current_order])){
                struct Node* allocated_address = list_pop(&order_list[current_order]);
                // mark the block as OCCUPIED
                the_array[(unsigned int)allocated_address->page_index].available = OCCUPIED;
                // the block_size start from zero, thus we need to -1
                the_array[(unsigned int)allocated_address->page_index].block_size = _pow(2, current_order)-1;
                // print the log
                uart_puts("[Malloc] block that contains ");
                uart_hex(_pow(2, current_order));
                uart_puts(" pages started from address 0x");
                uart_hex((unsigned int)allocated_address->address);
                uart_puts(" to address 0x");
                uart_hex((unsigned int)((allocated_address->address)+(_pow(2, current_order)-1)*PAGE_SIZE));
                uart_puts(" with index starts from ");
                uart_hex((unsigned int)allocated_address->page_index);
                uart_puts("\n");
                int cutter_count = 1;
                while(cut_freg > 0){
                    // mid address is the starting address of last page that BEEN malloced
                    int mid_page_index = allocated_address->page_index+(_pow(2, current_order-cutter_count))-1;
                    // end address is the starting address of last page in this block
                    unsigned char* end_address = mid_page_index+_pow(2, current_order-cutter_count);
                    Node* new_node = simple_malloc(sizeof(Node));
                    // push into order list
                    new_node->address = (unsigned char*) HEAP_START_ADDRESS+(PAGE_SIZE*(mid_page_index+1));
                    new_node->page_index = mid_page_index+1;
                    the_array[new_node->page_index].block_size = _pow(2, current_order-cutter_count)-1;
                    uart_puts("[Release redundant memory] block that contains ");
                    uart_hex(_pow(2, current_order-cutter_count));
                    uart_puts(" pages started from address 0x");
                    uart_hex(new_node->address);
                    uart_puts(" to address 0x");
                    uart_hex(new_node->address+((the_array[new_node->page_index].block_size)*PAGE_SIZE));
                    uart_puts(" is freed \n");
                    list_push_back(new_node, current_order-cutter_count);
                    cut_freg--;
                    cutter_count++;
                }
                if(cutter_count>1){
                    the_array[(unsigned int)allocated_address->page_index].block_size = _pow(2, current_order-(cutter_count-1))-1;
                }

                return allocated_address->address;
            }
            else{
                cut_freg++;  // we may allocate more then 2 times more memory than needed
                current_order++;
            }
        }
    }
    uart_puts("[Warning] malloc failed, returning 0x00\n");
    return 0x0;
}



void list_push_back(Node* to_push, int order){
    OrderList* target_list = &(order_list[order]);
    if(target_list->root == NULL){
        target_list->root = to_push;
    }
    else{
        Node* current = target_list->root;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = to_push;
    }
    //print_order_list(target_list);
}