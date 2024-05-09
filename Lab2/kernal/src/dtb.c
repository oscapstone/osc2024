#include "dtb.h"
#include "mini_uart.h"
#include "stdio.h"
#include "str.h"
#include "cpio.h"
void* _dtb_addr;
 
int align_mem_offset(void* i, unsigned int align){
    // unsigned long long seemed not work properly?
    uintptr_t l = (uintptr_t)i;
    return ((align - (l % align) ) % align);
}

uint32_t uint32_endian_big2little(uint32_t data)
{
	char *r = (char *)&data;
	return (r[3] << 0) | (r[2] << 8) | (r[1] << 16) | (r[0] << 24);
}

void initramfs_callback(char *struct_addr, char *string_addr, unsigned int prop_len){


    if(!strcmp(string_addr, "linux,initrd-start")){

        char *temp = struct_addr;
        temp += 4;
        if(prop_len > 0){
            puts("CPIO address:");
            // Since address are 64bits, if we declare int32, there will be warning
            uint64_t addr = (uint64_t)uint32_endian_big2little(*(uint32_t*)(temp));
            put_long_hex(addr);
            puts("\r\n");
            cpio_addr = (char*)addr;
        }
    }
}

void fdt_traverse(void (*callback)(char *, char *, unsigned int)){
    struct fdt_header* fdt = (struct fdt_header*)_dtb_addr;

    if(uint32_endian_big2little(fdt->magic) != 0xd00dfeed){
        puts("dtb magic value error\r\n");
        put_long_hex(uint32_endian_big2little(fdt->magic));
        puts("\r\n");
        return;
    }
    else{
        puts("dtb magic value ture\r\n");
    }
    unsigned int struct_size = uint32_endian_big2little(fdt->size_dt_struct);
    // for manipulation of pointer
    char* struct_ptr = (char*)fdt + uint32_endian_big2little(fdt->off_dt_struct);
    char* string_ptr = (char*)fdt + uint32_endian_big2little(fdt->off_dt_strings);

    while(struct_ptr < ((char*)fdt + uint32_endian_big2little(fdt->off_dt_struct) + struct_size)){
        uint32_t token = *(uint32_t*)struct_ptr;
        struct_ptr += 4;
        switch(uint32_endian_big2little(token)){
            case FDT_NOP:
                break;
            case FDT_BEGIN_NODE:
                unsigned int print_len = strsize(struct_ptr);
                struct_ptr += print_len;
                // as there's a NULL, so add 1
                struct_ptr++;
                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_PROP:
                // property value length 0 just indicate the property itself is sufficient(meaning that property name still exist)

                // property length
                unsigned int prop_len = uint32_endian_big2little(*(uint32_t*)(struct_ptr));
                // 32bits
                struct_ptr += 4;
                // property name offset (starting at string block)
                callback(struct_ptr, string_ptr + uint32_endian_big2little(*(uint32_t*)(struct_ptr)), prop_len);
                // 32bits
                struct_ptr += 4;
                // property value
                if(prop_len > 0){
                    struct_ptr += prop_len;
                }        

                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_END_NODE:
                break;
            case FDT_END:
                break;
            default:
                puts("error: wrong node type\r\n");
                return;
        }
    }
}
