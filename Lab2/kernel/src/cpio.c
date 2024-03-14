#include "cpio.h"
#include "string.h"
#include "uart.h"
#include "dtb.h"

static void *RAMFS = 0x0;
int hex2int(char *p, int len)
{
    int val = 0;
    int temp;
    for (int i = 0; i < len; i++)
    {
        temp = *(p + i);
        if (temp >= 'A')
        {
            temp = temp - 'A' + 10;
        }
        else
            temp -= '0';
        val *= 16;
        val += temp;
    }
    return val;
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

void initrd_fdt_callback(void *start, int size)
{
    if (size != 4)
    {
        uart_uint(size);
        uart_puts("Size not 4!\n");
        return 1;
    }
    uint32_t t = *((uint32_t *)start);
    RAMFS = (void *)(b2l_32(t));
}
int initrdGet()
{
    return RAMFS;
}
void cpioParse(char **ramfs, char *file_name, char *file_content)
{
    cpio_t header;
    int fileSize = 0;
    int nameSize = 0;
    read(ramfs, header.c_magic, 6);
    (*ramfs) += 48;
    read(ramfs, header.c_filesize, 8);
    (*ramfs) += 32;
    read(ramfs, header.c_namesize, 8);
    (*ramfs) += 8;
    nameSize = round2four(hex2int(header.c_namesize, 8), 1);
    fileSize = round2four(hex2int(header.c_filesize, 8), 2);
    read(ramfs, file_name, nameSize);
    read(ramfs, file_content, fileSize);
    file_name[nameSize] = '\0';
    file_content[fileSize] = '\0';
}

void cpioLs()
{
    char fileName[100];
    char fileContent[1000];
    char *ramfs = (char *)RAMFS;
    while (1)
    {
        strset(fileName, '0', 100);
        strset(fileContent, '0', 1000);
        cpioParse(&ramfs, fileName, fileContent);
        if (strcmp(fileName, "TRAILER!!!") == 0)
            break;
        uart_puts(fileName);
        uart_puts("\n");
    }
}

void cpioCat(char findFileName[])
{
    char fileName[100];
    char fileContent[1000];
    char *ramfs = (char *)RAMFS;
    int found = 0;
    while (1)
    {
        strset(fileName, '0', 100);
        strset(fileContent, '0', 1000);
        cpioParse(&ramfs, fileName, fileContent);
        if (strcmp(fileName, "TRAILER!!!") == 0)
            break;
        if (strcmp(fileName, findFileName) == 0)
        {
            found = 1;
            break;
        }
    }
    if (found)
    {
        uart_puts(fileContent);
    }
    else
    {
        uart_puts("FILE NOT FOUND!");
    }
    uart_puts("\n");
}
