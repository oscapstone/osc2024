#include "fdt.h"
#include "mini_uart.h"
#include <utils.h>
#include "helper.h"

char* _cpio_file;
char* _fdt_end;
char* fdt_addr;

void _print_tab(int level)
{
    while (level--) {
        uart_printf("\t");
    }
}

void _dump(char *start, int len)
{
	for(int i = 0; i < len; i ++) {
		if (start[i] >= 0x20 && start[i] <= 0x7e) uart_send(start[i]);
		else									  uart_printf("%x", start[i]);
	}
}

char* ALIGN(uint64_t ptr, int x) {
	if (ptr & 3) {
		ptr = (ptr + (4 - (ptr & 3)));	
	}
	return (char*)ptr;
}

uint32_t print_dtb(int type, char *name, char *data, uint32_t size)
{
	static int tb = 0;
	if (type == FDT_BEGIN_NODE) {
		_print_tab(tb);
		uart_printf("[*] Node: %s\r\n", name);
		tb ++;
	}
	else if(type == FDT_END_NODE) {
		tb --;
		_print_tab(tb);
		uart_printf("[*] Node: end\r\n");
	}
	else if(type == FDT_PROP) {
		_print_tab(tb);
		uart_printf("[*] %s: ", name);
		_dump(data, size);
		uart_printf("\r\n");
	}
	else if(type == FDT_NOP) {
		
	}
	else {
        uart_printf("[*] END!\r\n");
	}
	
    return 0;
}

uint32_t get_initramfs_addr(int type, char* name, char* data, uint32_t size)
{
	if (type == FDT_PROP && same(name, "linux,initrd-start")) {
		_cpio_file=(char *)(uintptr_t)fdt32_ld((void*)data);
    }
	return 0;
}

uint32_t parse_dt_struct(fdt_callback cb, char *dt_struct, char *dt_strings) {
	// inside a struct
    char *cur = dt_struct;
    
	while (1) {
        cur = (char *)ALIGN((uint64_t)cur, 4);
        struct fdt_node_header *nodehdr = (struct fdt_node_header *)cur;
        uint32_t tag = fdtn_tag(nodehdr), t = 0;
		
		if (tag == FDT_BEGIN_NODE) {
			t = cb(FDT_BEGIN_NODE, "", nodehdr -> name, -1);
            cur += sizeof(struct fdt_node_header) + strlen(nodehdr->name) + 1;
		}
		else if (tag == FDT_END_NODE) {
			t = cb(FDT_END_NODE, "", "", -1);
            cur += sizeof(struct fdt_node_header);
		}
		else if (tag == FDT_PROP) {
            struct fdt_property *prop = (struct fdt_property *)nodehdr;
            t = cb(FDT_PROP, dt_strings + fdtp_nameoff(prop), prop -> data, fdtp_len(prop));
            cur += sizeof(struct fdt_property);
            cur += fdtp_len(prop);
		}
		else if (tag == FDT_NOP) {
			t = cb(FDT_NOP, "", "", -1);
            cur += sizeof(struct fdt_node_header);
		}
		else {
			t = cb(FDT_END, "", "", -1);
			cur += sizeof(struct fdt_node_header);
        	break;
		}
		if ( t != 0 ) return t;
    }
	_fdt_end = cur;
	return 0;
}

uint32_t fdt_traverse(fdt_callback cb)
{
    struct fdt_header *hdr = (struct fdt_header *)fdt_addr;

    if (fdt_magic(hdr) != FDT_MAGIC) {
        uart_printf("[x] Not valid fdt_header\r\n");
    }

    char *dt_struct = fdt_addr + fdt_off_dt_struct(hdr);
    char *dt_strings = fdt_addr + fdt_off_dt_strings(hdr);

    uint32_t r = parse_dt_struct(cb, dt_struct, dt_strings);

	return r;
}

int get_fdt_end() {
	if (_fdt_end == 0) {
		uart_printf ("You haven't get fdt_end\r\n");
	}
	return _fdt_end;
}
