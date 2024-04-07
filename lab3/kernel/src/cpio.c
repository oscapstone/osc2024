#include "cpio.h"
#include "utils.h"
#include "uart.h"

FILE file_arr[MAX_FILE_SIZE];
int file_num = 0;
unsigned long long cpio_address;

void build_file_arr()
{
    int idx = 0;
    unsigned char *address = (unsigned char *)cpio_address;
    // When pathname is "TRAILER!!!". it means it's the end of file.
    while (my_strcmp((char *)(address + sizeof(FILE_HEADER)), "TRAILER!!!") != 0)
    {
        // Pointer to corresponded address.
        file_arr[idx].file_header = (FILE_HEADER *)address;
        unsigned long long header_path_name_size = sizeof(FILE_HEADER) + given_size_hex_atoi(file_arr[idx].file_header->c_namesize, 8);
        unsigned long long file_content_size = given_size_hex_atoi(file_arr[idx].file_header->c_filesize, 8);

        // Align to 4
        header_path_name_size += header_path_name_size % 4 == 0 ? 0 : 4 - header_path_name_size % 4;
        file_content_size += file_content_size % 4 == 0 ? 0 : 4 - file_content_size % 4;

        file_arr[idx].path_name = (char *)(address + sizeof(FILE_HEADER));
        file_arr[idx].file_content = (char *)(address + header_path_name_size);

        address += header_path_name_size + file_content_size;
        idx++;
    }
    file_num = idx;
};

void traverse_file()
{
    for (int i = 0; i < file_num; i++)
    {
        uart_puts(file_arr[i].path_name);
        uart_puts("\n");
    }
}

void look_file_content(char *pathname)
{
    for (int i = 0; i < file_num; i++)
    {
        if (my_strcmp(file_arr[i].path_name, pathname) == 0)
        {
            uart_puts(file_arr[i].file_content);
            uart_puts("\n");
        }
    }
}

void exec_program(char *pathname)
{
    for (int i = 0; i < file_num; i++)
    {
        if (my_strcmp(file_arr[i].path_name, pathname) == 0)
        {
            char *target = ((char *)(0x200000));
            char *content = file_arr[i].file_content;
            int size = given_size_hex_atoi(file_arr[i].file_header->c_filesize, 8);

            while (size--)
                *target++ = *content++;

            asm volatile(
                "mov x10, 0x0\n"
                "msr spsr_el1, x10\n"
                "mov x10, 0x200000\n"
                "msr elr_el1, x10\n"
                "msr sp_el0, x10\n"
                "eret\n");
        }
    }
}