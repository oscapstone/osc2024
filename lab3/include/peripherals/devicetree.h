#ifndef __DEVICETREE_H
#define __DEVICETREE_H

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009



typedef struct fdt_header {
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
} fdt_header;

struct fdt_reserve_entry {
    unsigned long long address;
    unsigned long long size;
};

int set_devicetree_addr(fdt_header *addr);

unsigned int read_bigendian(unsigned int *ptr);

int fdt_traverse(int (*fdt_callback)(char *, char *, char *, unsigned int));

#endif