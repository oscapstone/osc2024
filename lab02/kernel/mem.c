#include "mem.h"
#include "utils.h"


extern int _end;
#define HEAP_LIMIT 0x100+&_end
static char *head;

void mem_init(){
  head = (char *)&_end;
  head++;
}


void* simple_malloc(uint32_t size) {
  
  if (head + size > HEAP_LIMIT) return (char*)0;
  
  print_string("malloc from ");
  print_h(head);
  print_string(" to ");
  print_h(head+size-1);
  print_string(" under limit ");
  print_h((uint32_t)HEAP_LIMIT);
  print_char('\n');
  char* tail = head;
  head += size;
  // print_h(head);
  // print_char('\n');

  return tail;
}
