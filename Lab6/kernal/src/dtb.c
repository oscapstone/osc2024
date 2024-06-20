#include "dtb.h"
#include "mini_uart.h"
#include "stdio.h"
#include "str.h"
#include "cpio.h"
#include "peripherals/rpi_mmu.h"
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

uint64_t uint64_endian_big2little(uint64_t data)
{
    char *r = (char *)&data;
    return ((uint64_t)r[7] << 0)  | 
           ((uint64_t)r[6] << 8)  | 
           ((uint64_t)r[5] << 16) | 
           ((uint64_t)r[4] << 24) | 
           ((uint64_t)r[3] << 32) | 
           ((uint64_t)r[2] << 40) | 
           ((uint64_t)r[1] << 48) | 
           ((uint64_t)r[0] << 56);
}

void initramfs_callback(char *value_addr, char *string_addr, unsigned int prop_len){


    if(!strcmp(string_addr, "linux,initrd-start")){

        char *temp = value_addr;
        if(prop_len > 0){
            puts("prop_len:");
            put_int(prop_len);
            puts("\r\n");
            puts("CPIO address:");
            // Since address are 64bits, if we declare int32, there will be warning
            uint64_t addr = (uint64_t)uint32_endian_big2little(*(uint32_t*)(temp));
            addr = PHYS_TO_VIRT(addr);
            put_long_hex(addr);
            puts("\r\n");
            cpio_addr =(char*)addr;
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
        //puts("dtb magic value ture\r\n");
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
                callback(struct_ptr+4, string_ptr + uint32_endian_big2little(*(uint32_t*)(struct_ptr)), prop_len);
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

void fdt_output_reverse(){
    struct fdt_header* head = (struct fdt_header*)_dtb_addr;
    puts("magic:0x");
    put_hex(uint32_endian_big2little(head->magic));
    puts("\r\n");
    puts("totalsize:0x");
    put_hex(uint32_endian_big2little(head->totalsize));
    puts("\r\n");
    puts("off_dt_struct:0x");
    put_hex(uint32_endian_big2little(head->off_dt_struct));
    puts("\r\n");
    puts("off_dt_strings:0x");
    put_hex(uint32_endian_big2little(head->off_dt_strings));
    puts("\r\n");
    puts("off_mem_rsvmap:0x");
    put_hex(uint32_endian_big2little(head->off_mem_rsvmap));
    puts("\r\n");
    puts("size_dt_strings:0x");
    put_hex(uint32_endian_big2little(head->size_dt_strings));
    puts("\r\n");
    puts("size_dt_struct:0x");
    put_hex(uint32_endian_big2little(head->size_dt_struct));
    puts("\r\n");

    if(uint32_endian_big2little(head->magic) != 0xd00dfeed){
        puts("dtb magic value error\r\n");
        put_long_hex(uint32_endian_big2little(head->magic));
        puts("\r\n");
        return;
    }
    else{
        puts("dtb magic value ture\r\n");
    }

    struct fdt_reserve_entry* current=head+uint32_endian_big2little(head->off_mem_rsvmap);

    for(int i=1;;i++){
        puts("entry:");
        put_int(i);
        puts("\r\n");
        puts("address:0x");
        put_long_hex((unsigned long long)uint32_endian_big2little(current->address));
        puts("\r\n");
        puts("size:0x");
        put_long_hex((unsigned long long)uint32_endian_big2little(current->size));
        puts("\r\n");
        current++;
        if(uint64_endian_big2little(current->size) == 0){
            puts("5:0x");
            put_long_hex(5);
            puts("\r\n");
            puts("address:0x");
            put_long_hex((unsigned long long)uint32_endian_big2little(current->address));
            puts("\r\n");
            puts("size:0x");
            put_long_hex((unsigned long long)uint32_endian_big2little(current->size));
            puts("\r\n\r\n");
            break;
        }
    }
    puts("\r\n");
    // int* c=head+uint32_endian_big2little(head->off_mem_rsvmap);
    // int counter=0;
    // puts("struct block address:0x");
    // put_long_hex(head+uint32_endian_big2little(head->off_dt_struct));
    // puts("\r\n");
    // puts("mem reverse block address:0x");
    // put_long_hex(head+uint32_endian_big2little(head->off_mem_rsvmap));
    // puts("\r\n");
    // while(c < (head+uint32_endian_big2little(head->off_dt_struct)) & counter <=100  & 0){
    //     counter++;
    //     puts("c address:0x");
    //     put_long_hex(c);
    //     puts("\r\nc value:");
    //     put_int(*c);
    //     puts("\r\n");
    //     c++;
    // }
    // puts("\r\n");
}

unsigned long long dtb_end(){
    struct fdt_header* head = (struct fdt_header*)_dtb_addr;
    unsigned long long dtb_end=_dtb_addr+uint32_endian_big2little(head->totalsize);
    puts("dtb_start:0x");
    put_hex(_dtb_addr);
    puts("\r\n");
    puts("dtb_end:0x");
    put_hex(dtb_end);
    puts("\r\n");
    return dtb_end;
}