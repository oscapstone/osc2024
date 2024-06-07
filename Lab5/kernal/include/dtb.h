#ifndef DTB_H
#define DTB_H
// or use unsigned int instead?
#include <stdint.h>
#include <stddef.h>

// The structure block is composed of a sequence of pieces, each beginning with a token, that is, a big-endian 32-bit integer.
// Some tokens are followed by extra data
#define FDT_BEGIN_NODE 0x00000001   // marks the beginning of a node’s representation. It shall be followed by the node’s unit name as extra data. The name is stored as a null-terminated string The node name is followed by zeroed padding bytes, if necessary for alignment, and then the next token,
#define FDT_END_NODE   0x00000002   // The FDT_END_NODE token marks the end of a node’s representation. This token has no extra data; so it is followed immediately by the next token
#define FDT_PROP       0x00000003   // marks the beginning of the representation of one property in the devicetree. It shall be followed by extra data describing the property
#define FDT_NOP        0x00000004   // ignored by any program parsing the device tree. This token has no extra data; so it is followed immediately by the next token
#define FDT_END        0x00000009   // marks the end of the structure block. There shall be only one FDT_END token, and it shall be the last token in the structure block. It has no extra data

extern void* _dtb_addr;

// It consists of a small header, followed by three variable sized sections: the memory reservation block, the structure block, and the strings block 
struct fdt_header {
    uint32_t magic;                 // This field shall contain the value 0xd00dfeed (big-endian)
    uint32_t totalsize;             // This field shall contain the total size in bytes of the devicetree data structure. This size shall encompass all sections of the structure: the header, the memory reservation block, structure block and strings block, as well as any free space gaps between the blocks or after the final block.
    uint32_t off_dt_struct;         // This field shall contain the offset in bytes of the structure block from the beginning of the header.
    uint32_t off_dt_strings;        // This field shall contain the offset in bytes of the strings block from the beginning of the header
    uint32_t off_mem_rsvmap;        // This field shall contain the offset in bytes of the memory reservation block from the beginning of the header.
    uint32_t version;               // This field shall contain the version of the devicetree data structure.
    uint32_t last_comp_version;     // This field shall contain the lowest version of the devicetree data structure with which the version used is backwards compatible.
    uint32_t boot_cpuid_phys;       // This field shall contain the physical ID of the system’s boot CPU.
    uint32_t size_dt_strings;       // This field shall contain the length in bytes of the strings block section of the devicetree blob.
    uint32_t size_dt_struct;        // This field shall contain the length in bytes of the structure block section of the devicetree blob
};

struct fdt_reserve_entry { 
    uint64_t address;
    uint64_t size;
};

// The memory reservation block consists of a list of pairs of 64-bit big-endian integers
// Each pair gives the physical address and size in bytes of a reserved memory region. These given regions shall not overlap
// each other. The list of reserved blocks shall be terminated with an entry where both address and size are equal to 0.
// shall be located at an 8-byte aligned offset from the beginning of the devicetree blob

void initramfs_callback(char *struct_addr, char *string_addr, unsigned int prop_len);
void fdt_traverse(void (*callback)(char *, char*, unsigned int));
void fdt_output_reverse();
unsigned long long dtb_end();
#endif
