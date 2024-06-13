#ifndef __DEVICE_TREE_H
#define __DEVICE_TREE_H

#define FDT_HEADER_MAGIC 0xd00dfeed
/* structure block token */
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

struct fdt_header { // all unsigned int
    unsigned int magic; // should be 0xd00dfeed (big-endian)
    unsigned int totalsize; // in bytes
    unsigned int off_dt_struct; // offset in bytes of the structure block from  beginning of header
    unsigned int off_dt_strings; // " string block
    unsigned int off_mem_rsvmap; // " memory reservation block
    unsigned int version; 
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys; // physical ID of the system’s boot CPU
    unsigned int size_dt_strings;
    unsigned int size_dt_struct;
};

/*struct fdt_reserve_entry { // 64 bit big-endian
    unsigned long int address;
    unsigned long int size;
};*/

struct fdt_lex_prop { // 32 bit big-endian
    unsigned int len;
    unsigned int nameoff;
};

int dt_strcmp(char* a, char* b, int len);
unsigned int swap_endian(unsigned int b_end);
void get_dtb_addr();
void dev_tree_parser( void (*callback)(char *node_name, char *prop_name, struct fdt_lex_prop *prop) );

#endif