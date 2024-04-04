#ifndef _DTB_H
#define _DTB_H

#include "types.h"

#define FDT_BEGIN_NODE \
  0x00000001  // marks the beginnning of a node repersentation
#define FDT_END_NODE 0x00000002  // marks the ends of a node repersentation
#define FDT_PROP \
  0x00000003  // marks the beginning of the representation of one property in
              // the devicetree
#define FDT_NOP \
  0x00000004  // should be ignored by any program parsing the device tree
#define FDT_END \
  0x00000009  // marks the end of the structure block. There shall be only one
              // FDT_END token, and it shall be the last token in the structure
              // block.
#define UNUSED(x) (void)(x)

typedef void (*fdt_callback)(int type, const char *name, const void *data,
                             uint32_t size);

typedef struct {
  uint32_t magic;           // contain the value 0xd00dfeed (big-endian).
  uint32_t totalsize;       // in byte
  uint32_t off_dt_struct;   // the offset in bytes of the structure block
                            // from the beginning of the header
  uint32_t off_dt_strings;  // the offset in bytes of the strings block from
                            // the beginning of the header
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;  // the length in bytes of the strings block section
  uint32_t size_dt_struct;
} __attribute__((packed)) fdt_header;

int fdt_traverse(fdt_callback cb);
void get_cpio_addr(int token, const char *name, const void *data,
                   uint32_t size);
void print_dtb(int token, const char *name, const void *data, uint32_t size);

#endif