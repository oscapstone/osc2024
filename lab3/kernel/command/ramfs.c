#include "kernel/bsp/ramfs.h"

#include "all.h"
#include "kernel/io.h"
#include "kernel/memory.h"
#include "lib/stdlib.h"
#include "lib/string.h"

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
    if (argc < 2) {
      print_string("\nUsage: cat <file_name>\n");
      return;
    }
    char *file_name = argv[1];

    char *file_contents = ramfs_get_file_contents(file_name);
    uint32_t file_size = ramfs_get_file_size(file_name);

    if (file_contents) {
        print_string("\n");
        for (int i = 0; i < file_size; i++) {
            print_char(file_contents[i]);
        }
    } else {
        print_string("\nFile not found\n");
    }
}

struct Command cat_command = {.name = "cat",
                              .description = "print the contents of a file",
                              .function = &_cat_command};

void _exec_command(int argc, char **argv) {
    if (argc < 2) {
        print_string("\nUsage: exec <filename>");
        return;
    }

    char *file_name = argv[1];
    char *file_contents = ramfs_get_file_contents(file_name);
    uint32_t file_size = ramfs_get_file_size(file_name);
    if (file_contents == NULL) {
        print_string("[ERROR] File not found: ");
        print_string(file_name);
        print_string("\n");
        return;
    }
    print_string("\n[INFO] Executing file: ");
    print_string(file_name);
    print_string("\n");

    char *user_program = USER_PROGRAM_BASE;
    memncpy(user_program, file_contents, file_size);
    from_el1_to_el0(USER_PROGRAM_BASE, USER_STACK_POINTER_BASE);
}

struct Command exec_command = {.name = "exec",
                               .description = "execute a user program",
                               .function = &_exec_command};
