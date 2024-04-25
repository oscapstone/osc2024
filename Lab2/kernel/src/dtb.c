#include "dtb.h"
#include "uart.h"
#include "utils.h"

#define UNUSED(x) ( (void)(x) )

char* cpio_addr;
int space = 0;

void send_space(int n) {
	while(n--) uart_display_string("\t");
}

uint32_t fdt_u32_le2be (const void *addr) {
	const uint8_t *bytes = (const uint8_t *) addr;
	uint32_t ret = (uint32_t)bytes[0] << 24 |(uint32_t)bytes[1] << 16 |(uint32_t)bytes[2] << 8 |(uint32_t)bytes[3];
	return ret;
}

int fdt_traverse(fdt_callback cb, void* _dtb){
    uintptr_t dtb_ptr = (uintptr_t) _dtb;
    uart_send_char('\n');
    uart_send_char('\r');
    uart_display_string("dtb loading at ");
    uart_binary_to_hex(dtb_ptr);
    uart_send_char('\n');
    struct fdt_header* header = (struct fdt_header*) dtb_ptr;
	uint32_t magic = fdt_u32_le2be(&(header->magic));	
	
	if (magic != 0xd00dfeed){
        uart_send_char('\r');
		uart_display_string("The header magic is wrong\n");
		// return -1;
	}
    else{
        uart_send_char('\r');
        uart_display_string("magic == ");
        uart_binary_to_hex(magic);
        uart_send_char('\n');
    }

    uintptr_t struct_ptr = dtb_ptr + fdt_u32_le2be(&(header->off_dt_struct));
	uintptr_t strings_ptr = dtb_ptr + fdt_u32_le2be(&(header->off_dt_strings));
    uint32_t totalsize = fdt_u32_le2be(&header->totalsize);
    uart_send_char('\r');
    uart_display_string("Structure offset is ");
    uart_binary_to_hex(struct_ptr);
    uart_send_char('\n');
    uart_send_char('\r');
    uart_display_string("Strings offset is at");
    uart_binary_to_hex(strings_ptr);
    uart_send_char('\n');
    // uart_display_string("total size is ");
    // uart_binary_to_hex(totalsize);
    // uart_send_char('\n');
    uart_send_char('\n');

    struct_parse(cb, struct_ptr, strings_ptr, totalsize);

    return 0;
}

int struct_parse(fdt_callback cb, uintptr_t cur_ptr, uintptr_t strings_ptr,uint32_t totalsize){

    uintptr_t end_ptr = cur_ptr + totalsize;
    // int count = 0;
    while(cur_ptr < end_ptr){
        
        
        uint32_t token = fdt_u32_le2be((char*) cur_ptr);
        cur_ptr += 4;

        switch(token){
            case FDT_BEGIN_NODE:
                // count += 1;
                cb(token, (char*)cur_ptr, NULL, 0);
                cur_ptr += utils_align_up(utils_strlen((char*)cur_ptr),4);
                break;
            case FDT_END_NODE:
                cb(token, NULL, NULL,0);
                break;
            case FDT_PROP:{
                uint32_t len = fdt_u32_le2be((char*)cur_ptr);
                cur_ptr += 4;
                uint32_t nameoff = fdt_u32_le2be((char*)cur_ptr);
                cur_ptr += 4;
                cb(token,(char*)(strings_ptr + nameoff), (char*)cur_ptr, len);
                cur_ptr += utils_align_up(len,4);
                }
                break;
            case FDT_NOP:
                cb(token,NULL,NULL,0);
                break;
            case FDT_END:
                cb(token,NULL,NULL,0);
                break;
        }
        // if (count > 10 && token == FDT_END_NODE){
        //     break;
        // }
    }
    return 0;

}

void print_dtb(int token, const char* name, const void* data, uint32_t len) {
    // UNUSED(size);

	switch(token){
		case FDT_BEGIN_NODE:
			uart_display_string("\n");
            send_space(space);
			uart_display_string((char*)name);
			uart_display_string("{\n ");
            space++;
			break;
		case FDT_END_NODE:
			uart_display_string("\n");
            space--;
			if(space >0) send_space(space);
			uart_display_string("}\n");
			break;
		case FDT_NOP:
			break;
		case FDT_PROP:
            send_space(space);
			uart_display_string((char*)name);
            uart_display_string(" : ");
            data_parse((char*)name, data, len);
            uart_display_string("\n");
			break;
		case FDT_END:
			break;
	}
}

void get_cpio_addr(int token,const char* name,const void* data,uint32_t size){
	UNUSED(size);
	if(token==FDT_PROP && utils_string_compare((char *)name,"linux,initrd-start")){
		cpio_addr = (char*)(uintptr_t)fdt_u32_le2be(data);
        uart_send_char('\r');
		uart_display_string("cpio address is at: ");
		uart_binary_to_hex((uintptr_t)fdt_u32_le2be(data));
		uart_send_char('\n');
	}
}

void data_parse(const char* name, const void* data, uint32_t len){

    if(utils_keyword_compare(name, "microchip") || utils_string_compare(name, "reg") || utils_string_compare(name, "virtual-reg") || utils_string_compare(name, "ranges") || utils_string_compare(name, "size") || utils_string_compare(name, "reusable")){
        if(len == 0){
            uart_display_string("<empty>");
        }          
        while(len>0){
            uart_send_char('<');
            uart_binary_to_hex((uintptr_t)fdt_u32_le2be(data));
            uart_send_char('>');
            uart_send_char(' ');
            len -= 4;
        }

    }
    else if(utils_string_compare(name, "compatible") || utils_string_compare(name, "clocks") || utils_keyword_compare(name, "assigned") || utils_keyword_compare(name, "linux") || name[0] == '#' || utils_string_compare(name, "phandle") || utils_keyword_compare(name, "interrupt")){
        uart_send_char('<');
        uart_binary_to_hex((uintptr_t)fdt_u32_le2be(data));
        uart_send_char('>');
        uart_send_char(' ');
    }
    else{
        uart_display_string((char*) data);
    }
}
