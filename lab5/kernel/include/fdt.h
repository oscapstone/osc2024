// Ref: https://elixir.bootlin.com/linux/v5.16.14/source/scripts/dtc/libfdt/fdt.h
#ifndef _FDT_H
#define _FDT_H

#include <sys/types.h>
#include <stdint.h>

typedef int (*fdt_parser)(int level, char *cur, char *dt_strings);

void fdt_traversal(char *dtb);
void parse_dtb(char *dtb, fdt_parser parser);

typedef uint32_t fdt32_t;
typedef uint64_t fdt64_t;

/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 * Copyright 2012 Kim Phillips, Freescale Semiconductor.
 */

#ifndef __ASSEMBLY__

struct fdt_header {
    fdt32_t magic;             /* magic word FDT_MAGIC */
    fdt32_t totalsize;         /* total size of DT block */
    fdt32_t off_dt_struct;     /* offset to structure */
    fdt32_t off_dt_strings;    /* offset to strings */
    fdt32_t off_mem_rsvmap;    /* offset to memory reserve map */
    fdt32_t version;           /* format version */
    fdt32_t last_comp_version; /* last compatible version */

    /* version 2 fields below */
    fdt32_t boot_cpuid_phys;   /* Which physical CPU id we're
                                  booting on */
    /* version 3 fields below */
    fdt32_t size_dt_strings;   /* size of the strings block */

    /* version 17 fields below */
    fdt32_t size_dt_struct;    /* size of the structure block */
};

struct fdt_node_header {
    fdt32_t tag;
    char name[0];
};

struct fdt_property {
    fdt32_t tag;
    fdt32_t len;
    fdt32_t nameoff;
    char data[0];
};

#endif /* !__ASSEMBLY */

#define FDT_MAGIC      0xd00dfeed /* 4: version, 4: total size */
#define FDT_TAGSIZE    sizeof(fdt32_t)

#define FDT_BEGIN_NODE 0x1        /* Start node: full name */
#define FDT_END_NODE   0x2        /* End node */
#define FDT_PROP       0x3        /* Property: name off, size, content */
#define FDT_NOP        0x4        /* nop */
#define FDT_END        0x9

#define FDT_V1_SIZE    (7*sizeof(fdt32_t))
#define FDT_V2_SIZE    (FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE    (FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE   FDT_V3_SIZE
#define FDT_V17_SIZE   (FDT_V16_SIZE + sizeof(fdt32_t))

// Ref: https://elixir.bootlin.com/linux/v5.16.14/source/scripts/dtc/libfdt/libfdt.h#L249
#define fdt_get_header(fdt, field) \
    (fdt32_ld(&((const struct fdt_header *)(fdt))->field))
#define fdt_magic(fdt)              (fdt_get_header(fdt, magic))
#define fdt_totalsize(fdt)          (fdt_get_header(fdt, totalsize))
#define fdt_off_dt_struct(fdt)      (fdt_get_header(fdt, off_dt_struct))
#define fdt_off_dt_strings(fdt)     (fdt_get_header(fdt, off_dt_strings))
#define fdt_version(fdt)            (fdt_get_header(fdt, version))
#define fdt_last_comp_version(fdt)  (fdt_get_header(fdt, last_comp_version))
#define fdt_size_dt_strings(fdt)    (fdt_get_header(fdt, size_dt_strings))
#define fdt_size_dt_struct(fdt)     (fdt_get_header(fdt, size_dt_struct))

#define fdtn_get_header(fdtn, field) \
    (fdt32_ld(&((const struct fdt_node_header *)(fdtn))->field))
#define fdtn_tag(fdtn)              (fdtn_get_header(fdtn, tag))

#define fdtp_get_header(fdtp, field) \
    (fdt32_ld(&((const struct fdt_property *)(fdtp))->field))
#define fdtp_tag(fdtp)              (fdtp_get_header(fdtp, tag))
#define fdtp_len(fdtp)              (fdtp_get_header(fdtp, len))
#define fdtp_nameoff(fdtp)          (fdtp_get_header(fdtp, nameoff))

// Load fdt32 (big-endian)
static inline uint32_t fdt32_ld(const fdt32_t *p)
{
    const uint8_t *bp = (const uint8_t *)p;

    return ((uint32_t)bp[0] << 24)
        | ((uint32_t)bp[1] << 16)
        | ((uint32_t)bp[2] << 8)
        | bp[3];
}

typedef uint32_t (*fdt_callback)(int type, char *name, char *data, uint32_t size);
uint32_t print_dtb(int type, char *name, char *data, uint32_t size) ;
uint32_t get_initramfs_addr(int type, char *name, char *data, uint32_t size);
uint32_t fdt_traverse(fdt_callback cb);

int get_fdt_end();


#endif /* _FDT_H */
