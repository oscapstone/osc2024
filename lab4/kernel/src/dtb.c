#include "dtb.h"
#include "mini_uart.h"
#include "utils.h"
#include "cpio.h"

extern char *CPIO_START;
extern char *CPIO_END;
char *DTB_START;
char *DTB_END;

// Can check details from p.51, p.52 in device tree spec v0.4
struct fdt_header { 
    uint32_t magic;             // This field shoud contain 0xd00dfeed (big-endian)
    uint32_t totalsize;         // Total size in bytes of the devicetree data structure
    uint32_t off_dt_struct;     // The offset in bytes of the structure block
    uint32_t off_dt_strings;    // The offset in bytes of the strings block
    uint32_t off_mem_rsvmap;    // The offset in bytes of the memory reservation block
    uint32_t version;           // The version of the devicetree data structure
    uint32_t last_comp_version; // The lowest version of the devicetree data structure
    uint32_t boot_cpuid_phys;   // The physical ID of the system’s boot CPU
    uint32_t size_dt_strings;   // The length in bytes of the strings block section
    uint32_t size_dt_struct;    // The length in bytes of the structure block section
};

uint32_t big_to_little_endian(uint32_t data) {
    return ((data >> 24) & 0xFF)       
         | ((data >> 8) & 0xFF00)      
         | ((data << 8) & 0xFF0000)    
         | ((data << 24) & 0xFF000000);
}

void parse_dtb_tree(dtb_callback callback) {
    struct fdt_header* header = (struct fdt_header *)DTB_START;
    // Check magic number
    if(big_to_little_endian(header->magic) != 0xD00DFEED) {
        uart_puts("FDT header: Wrong magic number");
        return;
    }
    uint32_t total_struct_size = big_to_little_endian(header->size_dt_struct);
    char* struct_ptr = (char*)header + big_to_little_endian(header->off_dt_struct);
    char* string_ptr = (char*)header + big_to_little_endian(header->off_dt_strings);
    char* struct_tail = (char*)struct_ptr + total_struct_size;
    char* ptr = struct_ptr;
    
    while(ptr < struct_tail) {
        // Get the token
        uint32_t token = big_to_little_endian(*(uint32_t*)ptr);
        ptr += 4;

        if(token == FDT_BEGIN_NODE) {
            callback(token, ptr, 0, 0);
            ptr += strlen(ptr);
            // Alignment 4 byte
            ptr += 4 - (unsigned long long) ptr % 4;
        } 
        else if(token == FDT_END_NODE) {
            callback(token, 0, 0, 0);
        } 
        else if(token == FDT_PROP) {
            uint32_t len = big_to_little_endian(*(uint32_t*)ptr);
            ptr += 4;
            char* name = (char*)string_ptr + big_to_little_endian(*(uint32_t*)ptr);
            ptr += 4;
            callback(token, name, ptr, len);
            ptr += len;
            if((unsigned long long)ptr % 4 != 0) { 
                //alignment 4 byte
                ptr += 4 - (unsigned long long)ptr%4;
            }
        }
        else if(token == FDT_NOP) {
            callback(token, 0, 0, 0);
        }
        else if(token == FDT_END) {
            callback(token, 0, 0, 0);
        }
        else {
            uart_puts("error type:%x\n",token);
            return;
        }
    }

}

void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size) {
    // https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
    // linux,initrd-start will be assigned by start.elf based on config.txt

    if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0) {
		CPIO_START = (void *)(unsigned long long)big_to_little_endian(*(uint32_t *)value);
	}
	if (node_type == FDT_PROP && strcmp(name, "linux,initrd-end") == 0) {
		CPIO_END = (void *)(unsigned long long)big_to_little_endian(*(uint32_t *)value);
	}
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size) {
    static int level = 0;
    if(node_type==FDT_BEGIN_NODE) {
        for(int i=0;i<level;i++) uart_puts("   ");
        uart_puts("%s{\n",name);
        level++;
    } else if(node_type==FDT_END_NODE) {
        level--;
        for(int i=0;i<level;i++) uart_puts("   ");
        uart_puts("}\n");
    } else if(node_type==FDT_PROP){
        for(int i=0;i<level;i++) uart_puts("   ");
        uart_puts("%s\n",name);
    }
}

void dtb_init(void *dtb_ptr) {
	DTB_START = dtb_ptr;
	DTB_END = (char *)dtb_ptr + big_to_little_endian(((struct fdt_header*)dtb_ptr)->totalsize);
	parse_dtb_tree(dtb_callback_initramfs);
}