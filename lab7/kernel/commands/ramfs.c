
#include <bsp/entry.h>
#include <bsp/ramfs.h>
#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <lib/stdlib.h>
#include <lib/string.h>

void _ls_command(int argc, char **argv) {
    file_list_t *file_list = ramfs_get_file_list();
    for (int i = 0; i < file_list->file_count; i++) {
        puts("\n");
        puts(file_list->file_names[i]);
        puts(" ");
        print_d(file_list->file_sizes[i]);
        puts(" bytes");
    }
    puts("\n");
}

command_t ls_command = {.name = "ls",
                        .description = "list the files in ramfs",
                        .function = &_ls_command};

void _cat_command(int argc, char **argv) {
    if (argc < 2) {
        puts("\nUsage: cat <file_name>\n");
        return;
    }
    char *file_name = argv[1];

    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents) {
        puts("\n");
        for (int i = 0; i < file_size - 1; i++) {
            print_char(file_contents[i]);
        }
    } else {
        puts("\nFile not found\n");
    }
}

command_t cat_command = {.name = "cat",
                         .description = "print the contents of a file",
                         .function = &_cat_command};
