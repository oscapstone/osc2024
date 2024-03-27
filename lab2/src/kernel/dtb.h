#ifndef DTB_H
#define DTB_H

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef void (*fdt_callback)(int type,const char* name,const void *data,unsigned int size);

struct fdt_header {
    unsigned int magic;             // magic 0xd00dfeed (big endian)
    unsigned int totalsize;
    unsigned int off_dt_struct;     // the offset in bytes of the structure block from the beginning of the header
    unsigned int off_dt_strings;
    unsigned int off_mem_rsvmap;
    unsigned int version;
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys;
    unsigned int size_dt_strings;   // the length in bytes of the strings block section
    unsigned int size_dt_struct;
};

int fdt_traverse(fdt_callback cb);

#endif