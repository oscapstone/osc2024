#ifndef _DTB_H_
#define _DTB_H_

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009
#define FDT_MAGIC 0xd00dfeed

typedef void (*dtb_callback)(unsigned int node_type, char *name, void *value, unsigned int name_size);

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

unsigned int big2little_endian(unsigned int big);
void fdt_traverse(dtb_callback callback, char *dtb_base);
void initramfs_callback(unsigned int node_type, char *name, void *value, unsigned int name_size);
void find_reserved_memory();

#endif