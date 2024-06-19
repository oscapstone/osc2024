#ifndef _FDT_H
#define _FDT_H

typedef struct fdt_header {
    unsigned int magic;             // the value 0xd00dfeed (big-endian)
    unsigned int totalsize;         // the total size in bytes of the devicetree data structure
    unsigned int off_dt_struct;     // the offset in bytes of the structure block from the beginning of the header
    unsigned int off_dt_strings;    // the offset in bytes of the string block from the beginning of the header
    unsigned int off_mem_rsvmap;    // the offset in bytes of the memory reservation block from the beginning of the header
    unsigned int version;           // the version of the devicetree data structure
    unsigned int last_comp_version; // the lowest version of the devicetree data structure with which the version used is backwards compatible
    unsigned int boot_cpuid_phys;   // the physical ID of the system’s boot CPU
    unsigned int size_dt_strings;   // the length in bytes of the strings block section of the devicetree blob.
    unsigned int size_dt_struct;    // the length in bytes of the structure block section of the devicetree blob.
} fdt_t;

// lexical Lexical structure
// big-endian 32-bit integer, aligned on a 32-bit boundary
#define FDT_BEGIN_NODE  0x00000001  // beginning of a node’s representation -> node's unit name, next token may be any token except FDT_END
#define FDT_END_NODE    0x00000002  // end of a node’s representation, next token may be any token except FDT_PROP
#define FDT_PROP        0x00000003  // beginning of the representation of one property in the devicetree -> len(can be zero), nameoff(name offset in strings block), property’s value, next token may be any token except FDT_END
#define FDT_NOP         0x00000004  // ignore, next token, which can be any valid token 
#define FDT_END         0x00000009  // end of the structure block
typedef void (*dtb_callback)(char *name, void *addr);

void fdt_traverse(dtb_callback callback, void *dtb_addr);
void fdt_structure_parser(dtb_callback callback, char *cur_struct_ptr, char *addr_dt_strings, unsigned int size_dt_struct);

#endif
