#include "header/mem.h"
#include "header/utils.h"
#include "header/uart.h"

extern int _end;
#define HEAP_LIMIT 0x100 + &_end
static char *head;
char *HEAP_START = (char *) &_end;
char *HEAP_END = (char *)(&_end) + 0x100;

void mem_init() {
    head = (char *) &_end;
    head++;
}   

void* simple_malloc(uint32_t size) {
    if (head + size > HEAP_LIMIT) return (char*)0;
    
    uart_send_string("\r\n");
    uart_send_string("heap start: ");
    uart_hex((uint32_t)HEAP_START);
    uart_send_string("\r\n");
    uart_send_string("heap limit: ");
    uart_hex((uint32_t)HEAP_END);
    uart_send_string("\r\n");
    uart_send_string("malloc from ");
    uart_hex((uint32_t)head);
    uart_send_string(" to ");
    uart_hex((uint32_t)(head)+size-1);
    uart_send_string(" under limit ");
    uart_hex((uint32_t)HEAP_END);
    char* tail = head;
    head += size;
    // print_h(head);
    // print_char('\n');

    return tail;
}