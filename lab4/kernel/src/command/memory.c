#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/memory.h>

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
    if (ptr == 0) {
        print_string("\nMalloc failed! ");
    }

    print_string("\nMalloc address: ");
    print_h((unsigned int)ptr);
    print_string("\n");
}

struct Command test_malloc_command = {.name = "test_malloc",
                                      .description = "test malloc",
                                      .function = &_test_malloc_command};

void _test_kmalloc_comand(int argc, char **argv) {
    if (argc != 2) {
        print_string("\nUsage: test_kmalloc <size>\n");
        return;
    }

    unsigned int size = atoi(argv[1]);
    print_string("\nMalloc size: ");
    print_string(argv[1]);
    void *ptr = kmalloc(size);
    if (ptr == 0) {
        print_string("\nMalloc failed! ");
    }
    print_string("\nMalloc address: ");
    print_h((uint64_t)ptr);
    print_string(", ");
    print_d((uint64_t)ptr);
    print_string("\n");

    // print_kmalloc_caches();
    print_free_areas();
}

struct Command test_kmalloc_command = {.name = "test_kmalloc",
                                       .description = "test kmalloc",
                                       .function = &_test_kmalloc_comand};

void _test_kfree_command(int argc, char **argv) {
    if (argc != 2) {
        print_string("\nUsage: test_kfree <address>\n");
        return;
    }
    unsigned int addr = atoi(argv[1]);
    kfree((void *)addr);
    print_string("\nFree address: ");
    print_h(addr);
    print_string("\n");
    print_free_areas();
}

struct Command test_kfree_command = {.name = "test_kfree",
                                     .description = "test kfree",
                                     .function = &_test_kfree_command};
