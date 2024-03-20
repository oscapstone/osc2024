#include "../include/dtb.h"
#include "../include/cpio.h"
#include "../include/uart1.h"
#include "../include/utils.h"

extern void *CPIO_DEFAULT_PLACE;
char *dtb_ptr;

// store big endian
struct fdt_header
{
	uint32_t magic;
	uint32_t total_size;
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

void traverse_device_tree(void *dtb_ptr, dtb_callback callback)
{
	struct fdt_header *header = dtb_ptr;
	if (uint32_endian_big2little(header->magic) != 0xD00DFEED)
	{
		uart_puts("Traverse_device_tree : wrong magic in traverse_device_tree");
		return;
	}

	// get device tree size
	uint32_t struct_size = uint32_endian_big2little(header->size_dt_struct);

	char *dt_struct_ptr = (char *)((char *)header + uint32_endian_big2little(header->off_dt_struct));
	char *dt_strings_ptr = (char *)((char *)header + uint32_endian_big2little(header->off_dt_strings));
	char *end = (char *)dt_struct_ptr + struct_size;
	char *pointer = dt_struct_ptr;

	// traverse device tree's structure
	while (pointer < end)
	{
		// get type of node (pass the address into 'big2little' func)
		// cause the address stores the type of dt_struct, only need trans endian
		uint32_t token_type = uint32_endian_big2little(*(uint32_t *)pointer);

		pointer += 4; // pointer is a char pointer

		// if current node is beginning node
		if (token_type == FDT_BEGIN_NODE)
		{
			callback(token_type, pointer, 0, 0);			// pass 'node type' & 'ptr to node'
			pointer += strlen(pointer);						// mov pointer to the end of node
			pointer += 4 - (unsigned long long)pointer % 4; // align to 4-byte
		}

		// if current node is endding node
		else if (token_type == FDT_END_NODE)
		{
			callback(token_type, 0, 0, 0);
		}

		// if current node is property node
		else if (token_type == FDT_PROP)
		{
			uint32_t len = uint32_endian_big2little(*(uint32_t *)pointer);						  // get len
			pointer += 4;																		  // mov to begining
			char *name = (char *)dt_strings_ptr + uint32_endian_big2little(*(uint32_t *)pointer); // get ptr to name
			pointer += 4;
			callback(token_type, name, pointer, len);
			pointer += len;

			// since len might not be multiple of 4, check align
			if ((unsigned long long)pointer % 4 != 0)
			{
				pointer += 4 - (unsigned long long)pointer % 4;
			}
		}

		// if current node is NOP
		else if (token_type == FDT_NOP)
		{
			callback(token_type, 0, 0, 0);
		}

		// if current node is END
		else if (token_type == FDT_END)
		{
			callback(token_type, 0, 0, 0);
		}

		// if current node undefinded
		else
		{
			uart_puts("error type: %x\n", token_type);
			return;
		}
	}
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
	static int level = 0; // tabs depth
	if (node_type == FDT_BEGIN_NODE)
	{
		for (int i = 0; i < level; i++)
		{ // print tabs
			uart_puts("  ");
		}
		uart_puts("%s{\n", name);
		level++;
	}
	else if (node_type == FDT_END_NODE)
	{
		level--;
		for (int i = 0; i < level; i++)
		{
			uart_puts("  ");
		}
		uart_puts("}\n");
	}
	else if (node_type == FDT_PROP)
	{
		for (int i = 0; i < level; i++)
		{
			uart_puts("  ");
		}
		uart_puts("%s\n", name);
	}
}

void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
	if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0)
	{
		// get initramfs's address, then store it into CPIO_DEFAULT_PLACE
		CPIO_DEFAULT_PLACE = (void *)(unsigned long long)uint32_endian_big2little(*(uint32_t *)value);
	}
}