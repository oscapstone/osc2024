
#include "base.h"
#include "dtb.h"
#include "utils.h"
#include "uart.h"


extern void *_dtb_ptr;

int parse_struct(fdt_callback cb, UPTR cur_ptr, UPTR strings_ptr, U32 totalsize) {
    UPTR end_ptr = cur_ptr + totalsize;
	
	while(cur_ptr < end_ptr) {
		
		U32 token = utils_transferEndian((char *)cur_ptr);
		cur_ptr += 4;
		
		switch(token){
			case FDT_BEGIN_NODE:
			/*
			Token type (4 bytes): Indicates that it's an FDT_BEGIN_NODE token.
			Node name (variable length, NULL-terminated): Specifies the name of the node being opened.
			*/
				//uart_send_string("In FDT_BEGIN_NODE\n");
				cb(token, (char*)cur_ptr,NULL,0);	
				cur_ptr += utils_align_up(utils_strlen((char*)cur_ptr),4);
				break;
			case FDT_END_NODE:
			/*
			Token type (4 bytes): Indicates that it's an FDT_END_NODE token.
			*/
				//uart_send_string("In FDT_END_NODE;\n");
				cb(token,NULL,NULL,0);
				break;

			case FDT_PROP: {

			/*
			Token type (4 bytes): Indicates that it's an FDT_PROP token.
			Data length (4 bytes): Specifies the length of the property data (len).
			Name offset (4 bytes): Provides the offset of the property name within the strings block (nameoff).
			Property data (variable length): Contains the property data itself, the size of which is determined by len.
			*/
				//uart_send_string("In FDT_PROP \n");	
				U32 len = utils_transferEndian((char*)cur_ptr);
				cur_ptr += 4;
				U32 nameoff = utils_transferEndian((char*)cur_ptr);
				cur_ptr += 4;
				//second parameter name here is property name not node name
				cb(token,(char*)(strings_ptr + nameoff),(void*)cur_ptr,len);
				cur_ptr += utils_align_up(len,4);
				break;

			}
			case FDT_NOP:
				//uart_send_string("In FDT_NOP\n");
				cb(token,NULL,NULL,0);
				break;

			case FDT_END:
				//uart_send_string("In FDT_END\n");
				cb(token,NULL,NULL,0);
				return 0;
			default:;
				return -1;
		}
	}
	return -1;
}

int fdt_traverse(fdt_callback cb) {
    struct fdt_header* header = (struct fdt_header*) _dtb_ptr;

    unsigned int magic = utils_transferEndian(&header->magic);


    if (magic != 0xd00dfeed) {
        uart_send_string("DTB header not correct\n");
        uart_hex(header->magic);
        return -1;
    }

    UPTR struct_ptr = (UPTR)_dtb_ptr + utils_transferEndian(&header->off_dt_struct);
    UPTR strings_ptr = (UPTR)_dtb_ptr + utils_transferEndian(&header->off_dt_strings);
    U32 totalSize = utils_transferEndian(&header->totalsize);
	parse_struct(cb, struct_ptr, strings_ptr, totalSize);

    return 1;
}