#include <stdint.h>
#include <stddef.h>
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef void (*fdt_callback)(int type,const char* name,const void *data,uint32_t size);

//https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html
//The layout of the header for the devicetree is defined by the following C structure. 
//All the header fields are 32-bit integers, stored in big-endian format.
struct __attribute__((packed)) fdt_header {
    uint32_t magic;             // This field shall contain the value 0xd00dfeed (big-endian).
    uint32_t totalsize;         // in byte
    uint32_t off_dt_struct;     // the offset in bytes of the structure block from the beginning of the header
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;   // the length in bytes of the strings block section
    uint32_t size_dt_struct;
};

int fdt_traverse(fdt_callback cb,void *dtb_ptr);
void get_cpio_addr(int token,const char* name,const void* data,uint32_t size);
void print_dtb(int token, const char* name, const void* data, uint32_t size);
