#include "peripherals/devicetree.h"
#include "m_string.h"
#include "mini_uart.h"

fdt_header* devicetree_ptr;

int set_devicetree_addr(fdt_header *addr){
    devicetree_ptr = addr;
    return 0;
}

char* fdt_align(char *p, int len){
    int tail = len % 4;
    int pad = (4-tail)%4;
    return p + len + pad;
}

unsigned int read_bigendian(unsigned int *ptr){
    unsigned int ret = 0;
    unsigned int num = *ptr;
    for(int i=0; i<4; ++i){
        ret <<= 8;
        ret += num & 0x000000FF;
        num >>= 8;
    }
    return ret;
}

int fdt_traverse(int (*fdt_callback)(char *, char *, char *, unsigned int)){
    char* dt_addr = (char *)devicetree_ptr;
    char* dt_struct_addr =
        dt_addr + read_bigendian(&(devicetree_ptr->off_dt_struct));
    char* dt_strings_addr =
        dt_addr + read_bigendian(&(devicetree_ptr->off_dt_strings));
    
    char* ptr = dt_struct_addr;
    char* nodename;
    while(1){

        unsigned int token = read_bigendian((unsigned int*) ptr);
        ptr += 4;
        if(token == FDT_BEGIN_NODE){
            nodename = ptr;
            ptr = fdt_align(ptr, m_strlen(nodename) + 1);
        }
        else if(token == FDT_PROP){
            unsigned int proplen = read_bigendian((unsigned int*) ptr);
            ptr += 4;
            char *propname = dt_strings_addr + read_bigendian((unsigned int*) ptr);
            ptr += 4;
            char *propvalue = ptr;
            ptr = fdt_align(ptr, proplen);
            if(0 == fdt_callback(nodename, propname, propvalue, proplen)) break;
        }
        else if(token == FDT_END_NODE)
            continue;
        else if(token == FDT_NOP)
            continue;
        else if(token == FDT_END)
            break;
        else {
            uart_printf("%s:%d Device Tree: Unknown tag.\n", __FILE__, __LINE__);
        }
    }
    return 0;
}

