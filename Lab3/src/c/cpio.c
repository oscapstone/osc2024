// #include "printf.h"
#include "string.h"
#include "cpio.h"
#include "sys_regs.h"
#include "utils.h"
#include "uart.h"
#include "heap.h"

int hex2int(char *hex)
{
    int value = 0;
    for (int i = 0; i < 8; i++)
    {
        // get current character then increment
        char byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9')
            byte = byte - '0';
        else if (byte >= 'A' && byte <= 'F')
            byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        value = (value << 4) | byte;
    }
    return value;
}

int round2four(int origin, int option)
{
    int answer = 0;

    switch (option)
    {
    case 1:
        if ((origin + 6) % 4 > 0)
            answer = ((origin + 6) / 4 + 1) * 4 - 6;
        else
            answer = origin;
        break;

    case 2:
        if (origin % 4 > 0)
            answer = (origin / 4 + 1) * 4;
        else
            answer = origin;
        break;

    default:
        break;
    }

    return answer;
}

void read(char **address, char *target, int count)
{
    while (count--)
    {
        *target = **address;
        (*address)++;
        target++;
    }
}

void cpio_parse_header(char **ramfs, char *file_name, char *file_content)
{
    struct cpio_header header;
    int file_size = 0, name_size = 0;

    read(ramfs, header.c_magic, 6);
    (*ramfs) += 48;
    read(ramfs, header.c_filesize, 8);
    (*ramfs) += 32;
    read(ramfs, header.c_namesize, 8);
    (*ramfs) += 8;

    name_size = round2four(hex2int(header.c_namesize), 1);
    file_size = round2four(hex2int(header.c_filesize), 2);

    read(ramfs, file_name, name_size);
    read(ramfs, file_content, file_size);

    file_name[name_size] = '\0';
    file_content[file_size] = '\0';
}

void cpio_ls()
{
    char *addr = (char *)cpio_addr;

    // The end of the archive is indicated by a special record with the pathname	"TRAILER!!!".
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0)
    {

        struct cpio_header *header = (struct cpio_header *)addr;
        // get int pathname size
        unsigned long pathname_size = utils_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
        // header + pathname size
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;
        // get size of file
        unsigned long file_size = utils_atoi(header->c_filesize, (int)sizeof(header->c_filesize));

        // align to 4-byte
        // https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
        // The  pathname is followed by NUL bytes so that the total size of the fixed header plus pathname is a multiple of four.
        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        uart_puts(addr + sizeof(struct cpio_header));
        uart_puts("\n");

        addr += (headerPathname_size + file_size);
    }
}

void *cpio_find_file(char *name)
{
    char *addr = (char *)cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0)
    {
        if ((utils_string_compare((char *)(addr + sizeof(struct cpio_header)), name) != 0))
        {
            // find the file
            return addr;
        }

        // get next file
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned long pathname_size = utils_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        addr += (headerPathname_size + file_size);
    }
    return 0;
}

void cpio_cat(char *filename)
{
    char *target = cpio_find_file(filename);
    if (target)
    {
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        char *file_content = target + headerPathname_size;

        uart_send('\n');
        for (unsigned int i = 0; i < file_size; i++)
        {
            if (file_content[i] == '\0')
            {
                uart_send('\n');
            }
            else
            {
                uart_send(file_content[i]);
            }
        }
        uart_send('\n');
    }
    else
    {
        uart_puts("Not found the file\n");
    }
}

void cpio_run_executable(char executable_name[])
{
    // char *ramfs = (char *)0x8000000;
    char *target = cpio_find_file(executable_name);

    if (target)
    {
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        char *file_content = target + headerPathname_size;

        char *program_position = (char *)0x10A0000;
        /*char *program_position = (char *)kmalloc(file_size);
        unsigned long jump_addr = program_position;
        */
        while (file_size--)
        {
            *program_position = *file_content;
            program_position++;
            file_content++;
        }

        //current EL = 1 => using spsr_el1 and elr_el1
        unsigned long spsr_el1 = 0x3c0; //spsr_el1[3:0]=0000 => EL0t => change EL0 use EL0 sp
        asm volatile("msr spsr_el1, %0" ::"r"(spsr_el1));   //https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-?lang=en#fieldset_0-3_0
        unsigned long jump_addr = 0x10A0000;    //eret will jump to elr
        asm volatile("msr elr_el1, %0" ::"r"(jump_addr));
        unsigned long sp = (unsigned long)kmalloc(4096);
        asm volatile("msr sp_el0,%0" ::"r"(sp));    //因為要用EL0sp，所以要設sp_el0
        asm volatile("eret");   //It restores the processor state based on SPSR_ELn and branches to ELR_ELn, where n is the current exception level.
        
        /*
        asm volatile(
            "mov x0, 0x0\n"
            "msr spsr_el1, x0\n"
            //"mov x0, 0x10A0000\n"
            "msr elr_el1, %0\n"
            "mov x0, 0x60000\n"
            "msr sp_el0, x0\n"
            "eret\n"
            :
            : "r"(program_position));*/
    }
    else
    {
        uart_puts("Not found the file\n");
    }
}
