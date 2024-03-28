#include "kernel/memory.h"

#include "all.h"
#include "kernel/io.h"

void _test_malloc_command(int argc, char **argv) {
    print_string("\nMalloc size: ");
    unsigned int size = 0;
    while (1) {
        char c = read_c();
        if (c == '\n') {
            break;
        }
        size = size * 10 + (c - '0');
        print_char(c);
    }
    void *ptr = simple_malloc(size);
    if(ptr == 0) {
        print_string("\nMalloc failed! ");
    }

    print_string("\nMalloc address: ");
    print_h((unsigned int)ptr);
    print_string("\n");
}

struct Command test_malloc_command = {.name = "test_malloc",
                                      .description = "test malloc",
                                      .function = &_test_malloc_command};
