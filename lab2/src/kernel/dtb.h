#ifndef DTB_H
#define DTB_H

#include "base.h"

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef void (*fdt_callback)(int type,const char* name,const void *data,unsigned int size);

struct fdt_header {
    U32 magic;
    U32 totalsize;
    U32 off_dt_struct;
    U32 off_dt_strings;
    U32 off_mem_rsvmap;
    U32 version;
    U32 last_comp_version;
    U32 boot_cpuid_phys;
    U32 size_dt_strings;
    U32 size_dt_struct;
};

int fdt_traverse(fdt_callback cb);

#endif