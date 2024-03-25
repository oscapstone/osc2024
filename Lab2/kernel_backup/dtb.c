#include "uart.h"
#include "utils.h"
#include <stdint.h>

// Beginning of a node, followed by the node’s unit name as extra data
#define FDT_BEGIN_NODE 0x00000001
// End of a node, followed immediately by the next token
#define FDT_END_NODE 0x00000002
// Beginning of property, followed by property struct
#define FDT_PROP 0x00000003
// no extra data; followed immediately by the next token
#define FDT_NOP 0x00000004
// end of the structure block
#define FDT_END 0x00000009

typedef struct { 
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
} fdt_header;

typedef struct { 
    uint64_t address; 
    uint64_t size;
} fdt_reserve_entry;

typedef struct {
    uint32_t len;
    uint32_t nameoff; 
}fdt_prop;

uint32_t bswap_32(uint32_t num) {
    uint32_t swapped;
    swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
            ((num>>8)&0xff00) | // move byte 2 to byte 1
            ((num<<8)&0xff0000) | // move byte 1 to byte 2
            ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}

uint64_t bswap_64(uint64_t num) {
    uint64_t swapped;
    swapped = ((num>>56)&0xff) |
            ((num>>40)&0xff00) | 
            ((num>>24)&0xff0000) | 
            ((num>>8)&0xff000000) | 
            ((num<<8)&0xff00000000) | 
            ((num<<24)&0xff0000000000) |
            ((num<<40)&0xff000000000000) |
            ((num<<56)&0xff000000000000); 
    return swapped;
}

void dtb_list(void *dtb) {
    char *buf = (char *)dtb;
    fdt_header *header = (fdt_header*)dtb;
    uart_puts("\n");
    uart_hex(bswap_32(header->magic));

    if (bswap_32(header->magic)==0xd00dfeed)
    {
        uart_puts("Magic of dtb: ");
        uart_hex(bswap_32(header->magic));
        uart_puts("\nSize of dtb:");
        uart_hex(bswap_32(header->totalsize));
        uart_puts("\nOffset of Structure Block:");
        uart_int(bswap_32(header->off_dt_struct));
        uart_puts("\nOffset of Strings Block:");
        uart_int(bswap_32(header->off_dt_strings));
        uart_puts("\nOffset of Memory Reservation Block:");
        uart_int(bswap_32(header->off_mem_rsvmap));
        uart_puts("\nCurrent Version:");
        uart_int(bswap_32(header->version));
        uart_puts("\nLast Compatible Version:");
        uart_int(bswap_32(header->last_comp_version));
        uart_puts("\nBoot Processor ID:");
        uart_int(bswap_32(header->boot_cpuid_phys));
        uart_puts("\nLength in bytes of the strings block:");
        uart_int(bswap_32(header->size_dt_strings));
        uart_puts("\nLength in bytes of the structure block:");
        uart_int(bswap_32(header->size_dt_struct));
        char* str = buf + bswap_32(header->off_dt_strings);
        uart_puts("\n");
        buf += bswap_32(header->off_dt_struct);
        int pad = 0;
        int debug = 0;
        uint32_t* type;
        while(1){
            type = (uint32_t*) buf;
            if (bswap_32(*type)==FDT_BEGIN_NODE){
                pad = 0;
                buf += 4;
                if (bswap_32((uint32_t)buf) == 0) {
                    buf+=4;
                    continue;
                }
                while (*buf){
                    buf++;
                    pad++;
                }
                buf++;
                pad++;
                buf += (4-(pad%4))%4;
                continue;
            }
            else if (bswap_32(*type)==FDT_END_NODE){
                buf+=4;
            }
            else if (bswap_32(*type)==FDT_PROP){ //3
                buf += 4;
                fdt_prop *prop = (fdt_prop*)buf;
                uart_puts(" <");
                uart_puts(str + bswap_32(prop->nameoff));
                uart_puts(", ");
                buf += sizeof(fdt_prop); //8
                if (bswap_32(prop->len)==0) {
                    uart_puts("None>");
                    continue;
                }
                if (bswap_32(prop->len)==4) {
                    uart_hex(bswap_32(*buf));
                    uart_puts(">");
                    buf += bswap_32(prop->len);
                    continue;
                }
                uart_puts_l(buf, bswap_32(prop->len)-1);
                uart_puts(">");
                buf += bswap_32(prop->len);
                buf += (4-(bswap_32(prop->len)%4))%4;
                continue;
            }
            else if (bswap_32(*type)==FDT_END) {
                uart_puts("\n");
                return;
            }
            else if (bswap_32(*type)==FDT_NOP) {
                buf += 4;
                continue;
            }
            else {
                uart_int(*type);
                uart_puts("Error! at ");
                uart_hex(buf);
                buf += 4;
                continue;
            }
        }
    }
}

int find_dtb(void *dtb, const char* name, int name_len, void (*func)(void*)){
    char *buf = (char *)dtb;
    fdt_header *header = (fdt_header*)dtb;
    uart_puts("\n");
    uart_puts("Find Service ");
    uart_puts(name);

    if (bswap_32(header->magic)==0xd00dfeed)
    {
        char* str = buf + bswap_32(header->off_dt_strings);
        uart_hex((unsigned int) str);
        uart_puts("\n");
        buf += bswap_32(header->off_dt_struct);
        int pad = 0;
        int debug = 0;
        uint32_t* type;
        while(1){
            type = (uint32_t*) buf;
            if (bswap_32(*type)==FDT_BEGIN_NODE){
                pad = 0;
                buf += 4;
                if (bswap_32((uint32_t)buf) == 0) {
                    buf+=4;
                    continue;
                }
                while (*buf){
                    buf++;
                    pad++;
                }
                buf++;
                pad++;
                buf += (4-(pad%4))%4;
                continue;
            }
            else if (bswap_32(*type)==FDT_END_NODE){
                buf+=4;
            }
            else if (bswap_32(*type)==FDT_PROP){ //3
                buf += 4;
                fdt_prop *prop = (fdt_prop*)buf;
                buf += sizeof(fdt_prop); //8
                if (bswap_32(prop->len)==0) {
                    continue;
                }
                if (bswap_32(prop->len)==4) {
                    if(memcmp(str + bswap_32(prop->nameoff), name, name_len) == 0){
                        uart_puts("FIND");
                        func((void*)buf);
                    }
                    buf += bswap_32(prop->len);
                    continue;
                }
                buf += bswap_32(prop->len);
                buf += (4-(bswap_32(prop->len)%4))%4;
                continue;
            }
            else if (bswap_32(*type)==FDT_END) {
                uart_puts("\n");
                return;
            }
            else if (bswap_32(*type)==FDT_NOP) {
                buf += 4;
                continue;
            }
            else {
                uart_int(*type);
                uart_puts("Error! at ");
                uart_hex(buf);
                buf += 4;
                continue;
            }
        }
    }
    else {
        uart_puts("Service not Found!\n");
        return;
    }
}