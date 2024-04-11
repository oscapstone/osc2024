#ifndef _DTB_H
#define _DTB_H

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
                             unsigned int size);

typedef struct {
  unsigned int magic;           // contain the value 0xd00dfeed (big-endian).
  unsigned int totalsize;       // in byte
  unsigned int off_dt_struct;   // the offset in bytes of the structure block
                                // from the beginning of the header
  unsigned int off_dt_strings;  // the offset in bytes of the strings block from
                                // the beginning of the header
  unsigned int off_mem_rsvmap;
  unsigned int version;
  unsigned int last_comp_version;
  unsigned int boot_cpuid_phys;
  unsigned int
      size_dt_strings;  // the length in bytes of the strings block section
  unsigned int size_dt_struct;
} __attribute__((packed)) fdt_header;

int fdt_traverse(fdt_callback cb);
void get_cpio_addr(int token, const char *name, const void *data,
                   unsigned int size);
void print_dtb(int token, const char *name, const void *data,
               unsigned int size);

#endif