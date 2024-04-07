#include "../include/dtb.h"
#include "../include/mem_utils.h"
#include "../include/string_utils.h"
#include "../include/mini_uart.h"
#include "../include/cpio.h"

char *cpio_addr;

/* Lexical structure */
/* 1. FDT_BEGIN_NODE      0x00000001
      node's unit name
   
   2. FDT_END_NODE        0x00000002
   
   3. FDT_PROP_NODE       0x00000003
      uint32_t len        // len gives the length of the property’s value in bytes
      uint32_t nameoff    // nameoff gives an offset into the strings block
      (value)
   
   4. FDT_NOP             0x00000004

   5. FDT_END             0x00000009 
 */


/* convert from 32-bit big-endian to little-endian */
static uint32_t dtb_get_uint32(char *ptr)
{
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

/* a static function that query each node in structure block */
static unsigned int iterate_nodes(fdt_callback cb, char *struct_ptr, char *string_ptr, uint32_t struct_size)
{
    /* initialize the current (start) pointer and the end of pointer in the structre block */
    char *cur_ptr = struct_ptr;
    char *end_ptr = cur_ptr + struct_size;

    /* traverse each node in structre block */
    while (cur_ptr < end_ptr) {
        uint32_t token = dtb_get_uint32(cur_ptr);
        switch (token) {
        case FDT_BEGIN_NODE:
            char *nameptr = cur_ptr + 4;
            cb(token, nameptr, NULL, NULL);
            cur_ptr = mem_align(nameptr + my_strlen(nameptr) + 1, 4);         // the string should contained '\0' at the end
            break;
        case FDT_PROP:
            uint32_t data_len = dtb_get_uint32(cur_ptr + 4);
            uint32_t name_off = dtb_get_uint32(cur_ptr + 8);
            char *data_ptr = cur_ptr + 12;
            cb(token, string_ptr + name_off, data_ptr, data_len);
            cur_ptr = mem_align(data_ptr + data_len, 4);
            break;
        case FDT_END_NODE:
            cur_ptr += 4;
            break;
        case FDT_NOP:
            cur_ptr += 4;
            break;
        case FDT_END:
            return FDT_TRAVERSE_CORRECT;
            break;
        default:
            return FDT_TRAVERSE_FORMAT_ERROR;
        }
    }
    return FDT_TRAVERSE_CORRECT;
}

/* an interface that takes a callback function argument */
uint32_t fdt_traverse(fdt_callback cb, uint64_t dtb_addr)
{
    /* get the address of dtb and cast to pointert to fdt_header_t */
    // uart_hex(dtb_addr);
    // uart_send_string("\r\n");
    // uart_send_string("debug1\r\n");
    char *dtb_ptr = (char *)dtb_addr;
    fdt_header_t *header = (fdt_header_t *)dtb_ptr;

    /* check the magic number is  0xd00dfeed or not */
    // uart_send_string("0x");
    // uart_hex((*(uint32_t *)dtb_ptr));
    // uart_send_string("\r\n");
    if (dtb_get_uint32(&(header->magic)) != 0xd00dfeed)
        return  FDT_TRAVERSE_HEADER_ERROR;

    /* get the total size, pointer to struct block, pointer to string block and call callback function */
    uint32_t struct_size = dtb_get_uint32((char *) &(header->size_dt_struct));
    char *struct_ptr = dtb_ptr + dtb_get_uint32((char *) &(header->off_dt_struct));
    char *string_ptr = dtb_ptr + dtb_get_uint32((char *) &(header->off_dt_strings));
    return iterate_nodes(cb, struct_ptr, string_ptr, struct_size);
}

/* callback function in this lab */
void get_cpio_addr(uint32_t token, char *nameptr, char *dataptr, uint32_t v)
{
    if (token == FDT_PROP && my_strcmp((char *)nameptr, "linux,initrd-start") == 0) {
        cpio_addr = (char *)(uintptr_t)dtb_get_uint32((char *)dataptr);
    }
}