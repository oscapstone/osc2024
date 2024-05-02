#include "header/allocator.h"
#include "../header/mini_uart.h"
#include "../header/utils.h"

#define MEM_START 0x10000000

unsigned long *malloc_cur = (unsigned long *)MEM_START;

void *malloc(size_t size)
{
    align(&size,8);
    unsigned long *malloc_ret = malloc_cur;
    malloc_cur+=(unsigned int)size;
    return malloc_ret;
}
/*

 Initial Memory Layout:
+-----------------------+
| Address 0x10000000    | <--- malloc_cur starts here
+-----------------------+

 Step 1: Call malloc(sizeof("Hello")) - Request 6 bytes, aligned to 8 bytes
+-----------------------+ 0x10000000
| Reserved 8 bytes      | <-- malloc_ret points here, now referred as 'a'
| (Uninitialized)       |     malloc_cur moved to 0x10000008
+-----------------------+ 0x10000008

 Step 2: Initialize Memory with "Hello"
+-----------------------+ 0x10000000
| 'H' (a[0])            |
| 'e' (a[1])            |
| 'l' (a[2])            |
| 'l' (a[3])            |
| 'o' (a[4])            |
| '\0'(a[5])            |
| Padding (2 bytes)     | <--- Memory alignment padding
+-----------------------+ 0x10000008

 Continue Allocating...

 */