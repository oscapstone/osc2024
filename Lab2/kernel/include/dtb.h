#include <stddef.h>
#include <stdint.h>


#define FDT_BEGIN_NODE (0x00000001)
#define FDT_END_NODE (0x00000002)
#define FDT_PROP (0x00000003)
#define FDT_NOP (0x00000004)
#define FDT_END (0x00000009)

typedef void (*fdt_callback)(int node,const char* name,const void* data,uint32_t size);

uint32_t fdt_u32_le2be (const void *addr);
int fdt_traverse(fdt_callback cb, void* _dtb);
int struct_parse(fdt_callback cb, uintptr_t cur_ptr, uintptr_t strings_ptr,uint32_t totalsize);
void get_cpio_addr(int token,const char* name,const void* data,uint32_t size);
void print_dtb(int token, const char* name, const void* data, uint32_t size);
void data_parse(const char* name, const void* data, uint32_t len);



struct fdt_header {
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
};

struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};

struct fdt_property{
    uint32_t len;
    uint32_t nameoff;
};
