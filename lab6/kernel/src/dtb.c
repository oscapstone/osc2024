#include "dtb.h"
#include "uart1.h"
#include "cpio.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include "bcm2837/rpi_mmu.h"

extern char *CPIO_START;
extern char *CPIO_END;
char *DTB_START;
char *DTB_END;

// stored as big endian
struct fdt_header
{
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

uint32_t uint32_endian_big2little(uint32_t data)
{
	char *r = (char *)&data;
	return (r[3] << 0) | (r[2] << 8) | (r[1] << 16) | (r[0] << 24);
}

void dtb_init(void *dtb_ptr)
{
	DTB_START = dtb_ptr;
	DTB_END = (char *)dtb_ptr + uint32_endian_big2little(((struct fdt_header *)dtb_ptr)->totalsize);
	traverse_device_tree(dtb_callback_initramfs);
}

void traverse_device_tree(dtb_callback callback)
{
	struct fdt_header *header = (struct fdt_header *)DTB_START;
	if (uint32_endian_big2little(header->magic) != 0xD00DFEED)
	{
		puts("traverse_device_tree : wrong magic in traverse_device_tree");
		return;
	}
	// https://abcamus.github.io/2016/12/28/uboot%E8%AE%BE%E5%A4%87%E6%A0%91-%E8%A7%A3%E6%9E%90%E8%BF%87%E7%A8%8B/
	// https://blog.csdn.net/wangdapao12138/article/details/82934127
	uint32_t struct_size = uint32_endian_big2little(header->size_dt_struct);
	char *dt_struct_ptr = (char *)((char *)header + uint32_endian_big2little(header->off_dt_struct));
	char *dt_strings_ptr = (char *)((char *)header + uint32_endian_big2little(header->off_dt_strings));

	char *end = (char *)dt_struct_ptr + struct_size;
	char *pointer = dt_struct_ptr;

	while (pointer < end)
	{
		uint32_t token_type = uint32_endian_big2little(*(uint32_t *)pointer);

		pointer += 4;
		if (token_type == FDT_BEGIN_NODE)
		{
			callback(token_type, pointer, 0, 0);
			pointer += strlen(pointer);
			pointer += 4 - (uint64_t)pointer % 4; // alignment 4 byte
		}
		else if (token_type == FDT_END_NODE)
		{
			callback(token_type, 0, 0, 0);
		}
		else if (token_type == FDT_PROP)
		{
			uint32_t len = uint32_endian_big2little(*(uint32_t *)pointer);
			pointer += 4;
			char *name = (char *)dt_strings_ptr + uint32_endian_big2little(*(uint32_t *)pointer);
			pointer += 4;
			callback(token_type, name, pointer, len);
			pointer += len;
			if ((uint64_t)pointer % 4 != 0)
				pointer += 4 - (uint64_t)pointer % 4; // alignment 4 byte
		}
		else if (token_type == FDT_NOP)
		{
			callback(token_type, 0, 0, 0);
		}
		else if (token_type == FDT_END)
		{
			callback(token_type, 0, 0, 0);
		}
		else
		{
			puts("error type:");
			put_hex(token_type);
			puts("\n");
			return;
		}
	}
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
	static int level = 0;
	if (node_type == FDT_BEGIN_NODE)
	{
		for (int i = 0; i < level; i++)
			puts("   ");
		puts(name);
		puts("{\n");
		level++;
	}
	else if (node_type == FDT_END_NODE)
	{
		level--;
		for (int i = 0; i < level; i++)
			puts("   ");
		puts("}\n");
	}
	else if (node_type == FDT_PROP)
	{
		for (int i = 0; i < level; i++)
			puts("   ");
		puts(name);
		puts("\n");
	}
}

void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
	// https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
	// linux,initrd-start will be assigned by start.elf based on config.txt
	if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0)
	{
		CPIO_START = (void *)PHYS_TO_VIRT((uint64_t)uint32_endian_big2little(*(uint32_t *)value));
	}
	if (node_type == FDT_PROP && strcmp(name, "linux,initrd-end") == 0)
	{
		CPIO_END = (void *)PHYS_TO_VIRT((uint64_t)uint32_endian_big2little(*(uint32_t *)value));
	}
}