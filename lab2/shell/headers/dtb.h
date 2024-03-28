#ifndef __DTB_H
#define __DTB_H

#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

typedef void(*fdt_callback)(unsigned int type, char* name, void *data, unsigned int size);

struct fdt_header
{
    unsigned int magic;                 // 0xd00dfeed
    unsigned int totalsize;             // Total size of device tree.
    unsigned int off_dt_struct;         // offset of struct
    unsigned int off_dt_strings;        // offset of strings
    unsigned int off_mem_rsvmap;        // offset of memory reservation block
    unsigned int version;               // 
    unsigned int last_comp_version;     // 
    unsigned int boot_cpuid_phys;       // 
    unsigned int size_dt_strings;       // length of strings block section
    unsigned int size_dt_struct;        // length of structure block section
};

void fdt_traverse(fdt_callback, void*);
void initramfs_callback(unsigned int, char*, void*, unsigned int);

#endif