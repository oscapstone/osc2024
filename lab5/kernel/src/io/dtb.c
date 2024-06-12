

#include "dtb.h"

#include "base.h"
#include "io/uart.h"
#include "utils/utils.h"
#include "utils/printf.h"

extern void* _dtb_ptr;


int parse_struct(fdt_callback cb, struct fdt_header* header) {
    
#ifdef NS_DEBUG
    uart_send_string("reading information ...\r\n");
#endif
    UPTR structPtr = (UPTR)_dtb_ptr + (UPTR)utils_transferEndian(header->off_dt_struct);
    UPTR stringsPtr = (UPTR)_dtb_ptr + (UPTR)utils_transferEndian(header->off_dt_strings);
    U32 totalSize = utils_transferEndian(header->totalsize);

    NS_DPRINT("[DTB][TRACE] DTB TotalSize: %d bytes\n", totalSize);

    UPTR endPtr = (UPTR)header + totalSize;

    NS_DPRINT("[DTB][TRACE] Parsing token...\n");

    while (structPtr < endPtr) {
        U32 token = utils_transferEndian(*((int*)structPtr));
        structPtr += 4;

        //uart_send_string("Current Pointer: 0x");
        //uart_hex64(structPtr);
        //uart_send_string("\n");

        // uart_send_string("Token: 0x");
        // uart_binary_to_hex(token);
        // uart_send_string("\n");

        switch (token)
        {
        case FDT_BEGIN_NODE:
            {
                cb(token, (char*)structPtr, 0, 0);
                structPtr +=  utils_align_up(utils_strlen((char*)structPtr),4);
            }
            break;
        case FDT_END_NODE:
            {
                cb(token, 0, 0, 0);
            }
            break;

        case FDT_PROP:
        {
            //uart_send_string("Prop token!\n");
            U32 len = utils_transferEndian(*((U32*)structPtr));
            structPtr += 4;
            U32 nameoff = utils_transferEndian(*((U32*)structPtr));
            structPtr += 4;
            //second parameter name here is property name not node name
            cb(token, (char*)(stringsPtr + nameoff), (void*)structPtr, len);
            structPtr += utils_align_up(len, 4);
            break;

        }
        case FDT_NOP:
            //uart_send_string("In FDT_NOP\n");
            cb(token, 0, 0, 0);
            break;

        case FDT_END:
            //uart_send_string("In FDT_END\n");
            cb(token, 0, 0, 0);
            return 0;
        default:;
            return -1;
        }
    }

    return -1;
}

int fdt_traverse(fdt_callback cb) {
    struct fdt_header* header = (struct fdt_header*) _dtb_ptr;

    U32 magic = utils_transferEndian(header->magic);
    //uart_binary_to_hex(magic);
    //uart_send_string("\r\n");

    if (magic != 0xd00dfeed) {
        uart_send_string("DTB magic not correct");
        return -1;
    }
#ifdef NS_DEBUG
    uart_send_string("DTB magic correct\r\n");
#endif
    parse_struct(cb, header);

    return 1;
}

