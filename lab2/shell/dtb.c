#include"header/dtb.h"
#include"header/utils.h"
#include"header/uart.h"
extern char *cpio_start;
extern char *cpio_end;
char *dtb_base;
unsigned int endian_big2little(unsigned int x) {
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}
// a tree data structure which indicating what devices are on a computer system.
// only find out node of initramfs and get the address
void fdt_traverse(dtb_callback callback)
{
    struct fdt_header *header = (struct fdt_header *)dtb_base;
    // fdt header magic 0xD00DFEED (big-endian)
    if (endian_big2little(header->magic) != 0xD00DFEED)
    {
        uart_send_str("fdt_traverse: wrong magic in fdt_traverse\n");
        uart_send_str("expect: 0XD00DFEED, get: ");
        uart_send_char(endian_big2little(header->magic));
        uart_send_str("\n");
        return;
    }
 
    // length in bytes of structure block section of dtb
    unsigned int struct_size = endian_big2little(header->size_dt_struct);

    // check hackmd notes about the picture of DTB structure
    // header is address of fdt_header, so we need (char *)
    // offset in bytes of the structure block from beginning of header
    // to locate struct start
    char *dt_struct_ptr = (char *)((char *)header + endian_big2little(header->off_dt_struct));
    // offset in bytes of strings block from beginning of header
    // to locate string start
    // fdt_prop use string_ptr + nameoff to get the pathname
    char *dt_strings_ptr = (char *)((char *)header + endian_big2little(header->off_dt_strings));

    // parse from struct begin to end
    char *end = (char *)dt_struct_ptr + struct_size;
    char *pointer = dt_struct_ptr;

    // according to lexical structure
    while (pointer < end)
    {
        // lexical big-endian-32-bit integer
        // all tokens shall be alligned on 32-bit boundary
        unsigned int token_type = endian_big2little(*(unsigned int *)pointer);
        pointer += 4;

        // lexical structure
        switch (token_type)
        {
        // begin of node's representation
        case FDT_BEGIN_NODE:
            // move node's unit name
            // string end \0
            pointer += strlen(pointer);
            // node name is followed by zeroed padding bytes
            // allign
            pointer += (4 - (unsigned long long)pointer % 4);
            break;

        // end of node's representation
        case FDT_END_NODE:
            break;
        
        case FDT_PROP:

            // len | name offset | address
            // uint32_t
            // length of prop values in byte
            unsigned int len = endian_big2little(*(unsigned int *)pointer);
            pointer += 4;

            // nameoff save offset of string blocks
            // strings_ptr + nameoff get the name
            char *name = (char *)dt_strings_ptr + endian_big2little(*(unsigned int *)pointer);
            pointer += 4;

            // check node is initrd-start/end and set cpio_start/end address
            callback(token_type, name, pointer, len);
            // address, byte string of length len
            pointer += len;
            // followed by zeroed padding bytes
            if ((unsigned long long)pointer % 4 != 0)
                pointer += 4 - (unsigned long long)pointer % 4; // alignment 4 byte
            break;
        // ** cant skip
        // ignore NOP
        case FDT_NOP:
            break;
        // marks end of structures block
        case FDT_END:
            break;
        default:
            return;
        }
    }
}

void initramfs_callback(unsigned int node_type, char *name, void *value, unsigned int name_size)
{
    if (string_compare(name, "linux,initrd-start")){
        cpio_start = (char *)(unsigned long long)endian_big2little(*(unsigned int *)value);
    }
    if (string_compare(name, "linux,initrd-end")){
        cpio_end = (char *)(unsigned long long)endian_big2little(*(unsigned int *)value);
    }
}