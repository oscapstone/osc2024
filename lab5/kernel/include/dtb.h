
#include "uint.h"

struct fdt_header {
    unsigned int magic;
    unsigned int totalsize;
    unsigned int off_dt_struct;
    unsigned int off_dt_strings;
    unsigned int off_mem_rsvmap;
    unsigned int version;
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys;
    unsigned int size_dt_strings;
    unsigned int size_dt_struct;
};
extern char* cpio_start;
extern char* cpio_end;

#define FDT_BEGIN_NODE  0x1             /* Start node: full name */
#define FDT_END_NODE    0x2             /* End node */
#define FDT_PROP        0x3             /* Property: name off, size, content */
#define FDT_NOP         0x4             /* nop */
#define FDT_END         0x9
typedef void (*dtb_callback)(unsigned int node_type, char *name, void *value, unsigned int name_size);

unsigned int endian_big2little(unsigned int x);
void fdt_traverse(dtb_callback callback);
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void initramfs_callback(unsigned int node_type, char *name, void *value, unsigned int name_size);