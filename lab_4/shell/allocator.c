#include "include/allocator.h"
#include "include/utils.h"
#include "include/uart.h"

int the_array[AVAILABLE_PAGE_NUM];
OrderList order_list[MAX_ORDER];


/*Pop the top element in linked list, note I assume there EXIST AT LEAST ONE AVAILABLE NODE*/
struct Node* list_pop(OrderList* target){
    Node* to_ret = target->root;
    while(to_ret -> next != NULL){
        to_ret = to_ret->next;
    }
    target->root = target->root->next;
    return to_ret;
}

void init_order_list(){
    int current_order = MAX_ORDER-1;
    int current_page_index = 0;

    // initialize order_list
    for(int i=0;i<MAX_ORDER;i++){
        order_list[i].chain_length=0;
        order_list[i].root = NULL;
    }

    // assume all pages are available at beginnig, greedly allocate pages in max order size
    while(current_order >= 0){
        while((current_page_index+_pow(2, current_order)-1) < AVAILABLE_PAGE_NUM){
            unsigned int allocated_size = _pow(2, current_order)*PAGE_SIZE;
            // record the grouped block at The Array
            the_array[current_page_index] = _pow(2, current_order);
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
    // for(int i=0;i<MAX_ORDER;i++){
    //     uart_puts("\nin list number ");
    //     uart_hex(i);
    //     uart_puts("\n");
    //     print_order_list(&order_list[i]);
    // }

}

void print_order_list(OrderList* list){
    Node* cur = list->root;
    while(cur != NULL){
        uart_puts("address ");
        uart_hex(cur->address);
        uart_puts("\n");
        uart_hex(cur->page_index);
        uart_puts("\n");
        cur = cur->next;
        
    }
}

void init_buddy_system(){
    init_order_list();
}

int check_list_has_element(OrderList* target){
    return target->root != NULL;
}

unsigned char* malloc_page(int request_page_num){
    //determine the minimum list order that this request needs
    int current_order = 0;
    int cut_freg = 0;
    // start from allocating the smallest block until is can satisfy needs
    
    while(current_order<MAX_ORDER){
        if(request_page_num > _pow(2, current_order)){
            current_order++;
        }
        else{
            if(check_list_has_element(&order_list[current_order])){
                struct Node* allocated_address = list_pop(&order_list[current_order]);
                // mark the block as OCCUPIED
                // the_array[(unsigned int)allocated_address->page_index] = OCCUPIED;
                //uart_hex((unsigned int)allocated_address->address-HEAP_START_ADDRESS);
                // print the log
                uart_puts("[Malloc] block that contains ");
                uart_hex(_pow(2, current_order));
                uart_puts(" pages started from address 0x");
                uart_hex((unsigned int)allocated_address->address);
                uart_puts(" to address 0x");
                uart_hex((unsigned int)((allocated_address->address)+_pow(2, current_order)*PAGE_SIZE));
                uart_puts(" with index starts from ");
                uart_hex((unsigned int)allocated_address->page_index);
                uart_puts("\n");
                int cutter_count = 1;
                while(cut_freg > 0){
                    int mid_page_index = allocated_address->page_index+(_pow(2, current_order-cutter_count));
                    unsigned char* end_address = mid_page_index+_pow(2, current_order-cutter_count);
                    the_array[mid_page_index+1] = _pow(2, current_order-cutter_count);
                    Node* new_node = simple_malloc(sizeof(Node));
                    // push into order list
                    new_node->address = (unsigned char*) HEAP_START_ADDRESS+(PAGE_SIZE*(mid_page_index+1));
                    new_node->page_index = mid_page_index+1;
                    uart_puts("[Release redundant memory] block that contains ");
                    uart_hex(_pow(2, current_order-cutter_count));
                    uart_puts(" pages started from address 0x");
                    uart_hex(new_node->address);
                    uart_puts(" to address 0x");
                    uart_hex(end_address);
                    uart_puts(" is freed \n");
                    list_push_back(new_node, current_order-cutter_count);
                    cut_freg--;
                    cutter_count++;
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
}