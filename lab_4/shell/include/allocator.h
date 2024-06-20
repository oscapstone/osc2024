#define HEAP_START_ADDRESS 0x10000000
#define HEAP_END_ADDRESS 0x20000000
#define PAGE_SIZE 0x1000
#define AVAILABLE_PAGE_NUM (HEAP_END_ADDRESS-HEAP_START_ADDRESS)/PAGE_SIZE
#define MAX_ORDER 11
#define FREE -1
#define OCCUPIED -2
#define NULL 0

typedef struct Node{
    unsigned char* address;
    int page_index; // the index of node in The Array
    struct Node* next;
}Node;

typedef struct OrderList{
    int chain_length;
    struct Node* root;
}OrderList;

void init_buddy_system();
void print_order_list(OrderList* list);
struct Node* list_pop(OrderList* target);
void init_order_list();
unsigned char* malloc_page(int request_page_num);
int check_list_has_element(OrderList* target);
void list_push_back(Node* to_push, int order);