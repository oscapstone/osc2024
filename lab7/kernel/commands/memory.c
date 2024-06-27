#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <lib/stdlib.h>

void _test_malloc_command(int argc, char **argv) {
    puts("\nMalloc size: ");
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
        puts("\nMalloc failed! ");
    }

    puts("\nMalloc address: ");
    print_h((uintptr_t)ptr);
    puts("\n");
}

command_t test_malloc_command = {.name = "test_malloc",
                                      .description = "test malloc",
                                      .function = &_test_malloc_command};

void _test_kmalloc_comand(int argc, char **argv) {
    if (argc != 2) {
        puts("\nUsage: test_kmalloc <size>\n");
        return;
    }

    int size = atoi(argv[1]);
    puts("\nMalloc size: ");
    puts(argv[1]);
    void *ptr = kmalloc(size);
    if (ptr == 0) {
        puts("\nMalloc failed! ");
    }
    puts("\nMalloc address: ");
    print_h((uint64_t)ptr);
    puts(", ");
    print_d((uint64_t)ptr);
    puts("\n");

    // print_kmalloc_caches();
    // print_free_areas();
}

command_t test_kmalloc_command = {.name = "test_kmalloc",
                                       .description = "test kmalloc",
                                       .function = &_test_kmalloc_comand};

void _test_kfree_command(int argc, char **argv) {
    if (argc != 2) {
        puts("\nUsage: test_kfree <address>\n");
        return;
    }
    uintptr_t addr = atoi(argv[1]);
    kfree((void *)addr);
    puts("\nFree address: ");
    print_h(addr);
    puts("\n");
    print_free_areas();
}

command_t test_kfree_command = {.name = "test_kfree",
                                     .description = "test kfree",
                                     .function = &_test_kfree_command};
