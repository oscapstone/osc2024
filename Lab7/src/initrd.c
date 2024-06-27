#include "initrd.h"
#include "string.h"
#include "mini_uart.h"
#include "devicetree.h"

static void *archive_start = (void *)0x0; // 0x08000000
static void *archive_end = (void *)0x0;
static int exec_size = 0;

void initrd_list()
{
  char *current = (char *)archive_start;
  cpio_header *head;

  while (!memcmp(current, "070701", 6) && memcmp(current + sizeof(cpio_header), "TRAILER!!!", 10))
  {
    char filename[256];
    head = (cpio_header *)current;
    int filesize = hstr2int(head->c_filesize, 8);
    int namesize = hstr2int(head->c_namesize, 8);
    int padding = (4 - ((filesize + namesize + sizeof(cpio_header)) % 4)) % 4;

    for (int i = 0; i < namesize; i++)
    {
      filename[i] = *(current + sizeof(cpio_header) + i);
    }
    filename[namesize] = '\0';
    uart_sends(filename);
    uart_sendc('\n');

    current += (filesize + namesize + sizeof(cpio_header) + padding);
  }
}

void initrd_cat(char *filename)
{
  char *current = (char *)archive_start;
  cpio_header *head;

  while (!memcmp(current, "070701", 6) && memcmp(current + sizeof(cpio_header), "TRAILER!!!", 10))
  {
    head = (cpio_header *)current;
    int filesize = hstr2int(head->c_filesize, 8);
    int namesize = hstr2int(head->c_namesize, 8);
    int n_padding = (4 - ((namesize + sizeof(cpio_header)) % 4)) % 4;
    int f_padding = (4 - filesize % 4) % 4;

    if (!strcmp(filename, current + sizeof(cpio_header)))
    {
      for (int i = 0; i < filesize; i++)
        uart_sendc(*(current + sizeof(cpio_header) + namesize + n_padding + i));
      uart_sendc('\n');
      return;
    }
    current += (filesize + namesize + sizeof(cpio_header) + n_padding + f_padding);
  }
  uart_sends("cat: ");
  uart_sends(filename);
  uart_sends(" No such file or directory\n");
}

void initramfs_callback(void *node, char *propname)
{
  if (!strcmp(propname, "linux,initrd-start"))
  {
    uint32_t tmp = *((uint32_t *)node);
    archive_start = (void *)((uintptr_t)swap32(tmp));
  }
  if (!strcmp(propname, "linux,initrd-end"))
  {
    uint32_t tmp = *((uint32_t *)node);
    archive_end = (void *)((uintptr_t)swap32(tmp));
  }
}

void *fetch_exec(char *filename)
{
  char *current = (char *)phys_to_virt(archive_start);
  cpio_header *head;

  while (!memcmp(current, "070701", 6) && memcmp(current + sizeof(cpio_header), "TRAILER!!!", 10))
  {
    head = (cpio_header *)current;
    int filesize = hstr2int(head->c_filesize, 8);
    int namesize = hstr2int(head->c_namesize, 8);
    int n_padding = (4 - ((namesize + sizeof(cpio_header)) % 4)) % 4;
    int f_padding = (4 - filesize % 4) % 4;

    if (!strcmp(filename, current + sizeof(cpio_header)))
    {
      // exec_file = (void *)(current+sizeof(cpio_header)+namesize+n_padding);
      exec_size = filesize;
      return (void *)(current + sizeof(cpio_header) + namesize + n_padding);
    }
    current += (filesize + namesize + sizeof(cpio_header) + n_padding + f_padding);
  }
  return ((void *)0);
}

void *get_initrd_start()
{
  return archive_start;
}

void *get_initrd_end()
{
  return archive_end;
}

int get_exec_size()
{
  return exec_size; // size of the executable file
}
