#include "devtree.h"
#include "uart.h"
#include "utils.h"
#include "string.h"

// dtb_addr is defined in start.S
extern void *dtb_addr;

/**
 * @brief Convert a 4-byte little-endian sequence to big-endian.
 * 
 * @param s: little-endian sequence
 * @return big-endian sequence
 */
static uint32_t le2be(const void *s)
{
	const uint8_t *bytes = (const uint8_t *)s;
	return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
	       (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}

void fdt_traverse(void (*callback)(void *))
{
	uart_puts("[INFO] Dtb is loaded at ");
	uart_hex((uintptr_t)dtb_addr);
	uart_putc('\n');

	uintptr_t dtb_ptr = (uintptr_t)dtb_addr;
	struct fdt_header *header = (struct fdt_header *)dtb_ptr;

	// Check the magic number
	if (le2be(&(header->magic)) != 0xD00DFEED)
		uart_puts("[ERROR] Dtb header magic does not match!\n");

	uintptr_t structure = (uintptr_t)header + le2be(&header->off_dt_struct);
	uintptr_t strings = (uintptr_t)header + le2be(&header->off_dt_strings);
	uint32_t totalsize = le2be(&header->totalsize);

	// Parse the structure block
	uintptr_t ptr = structure; // Point to the beginning of structure block
	while (ptr < strings + totalsize) {
		uint32_t token = le2be((char *)ptr);
		ptr += 4; // Token takes 4 bytes

		switch (token) {
		case FDT_BEGIN_NODE:
			ptr += align4(strlen((char *)ptr));
			break;
		case FDT_END_NODE:
			break;
		case FDT_PROP:
			uint32_t len = le2be((char *)ptr);
			ptr += 4;
			uint32_t nameoff = le2be((char *)ptr);
			ptr += 4;
			if (!strcmp((char *)(strings + nameoff),
				    "linux,initrd-start")) {
				callback(le2be(ptr));
			}
			ptr += align4(len);
			break;
		case FDT_NOP:
			break;
		case FDT_END:
			break;
		}
	}
}
