#include "kernel/bsp/ramfs.h"

#include "all.h"
#include "kernel/io.h"

void _ls_command(int argc, char **argv) {
    FileList *file_list = ramfs_get_file_list();
    for (int i = 0; i < file_list->file_count; i++) {
        print_string("\n");
        print_string(file_list->file_names[i]);
    }
    print_string("\n");
}

struct Command ls_command = {.name = "ls",
                             .description = "list the files in ramfs",
                             .function = &_ls_command};

void _cat_command(int argc, char **argv) {
    // TODO: implment argc, argv feature
    // if (argc < 2) {
    //   print_string("\nUsage: cat <file_name>\n");
    //   return;
    // }
    // char *file_name = argv[1];
    print_string("\nFile Name: ");
    char file_name[256];
    read_s(file_name);

    char *file_contents = ramfs_get_file_contents(file_name);
    if (file_contents) {
        print_string("\n");
        print_string(file_contents);
    } else {
        print_string("\nFile not found\n");
    }
}

struct Command cat_command = {.name = "cat",
                              .description = "print the contents of a file",
                              .function = &_cat_command};
