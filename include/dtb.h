#ifndef _DTB_H
#define _DTB_H

typedef void (*fdt_callback)(int type, const char *name, const void *data, unsigned int size);

typedef struct {
    unsigned int magic;          // contain the value 0xd00dfeed (big-endian).
    unsigned int totalsize;      // in byte
    unsigned int off_dt_struct;  // the offset in bytes of the structure block from the beginning of the header
    unsigned int off_dt_strings; // the offset in bytes of the strings block from the beginning of the header
    unsigned int off_mem_rsvmap;
    unsigned int version;
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys;
    unsigned int size_dt_strings; // the length in bytes of the strings block section
    unsigned int size_dt_struct;
} __attribute__((packed)) fdt_header;

int fdt_traverse(fdt_callback cb);
void get_cpio_addr(int token, const char *name, const void *data, unsigned int size);
void print_dtb(int token, const char *name, const void *data, unsigned int size);

#endif