#ifndef DTB_H
#define DTB_H

#define uint8_t unsigned char
#define uint32_t unsigned int
#define uint64_t unsigned long

typedef struct {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
} fdt_header;

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

#define FDT_MAGIC 0xd00dfeed

typedef struct {
  uint32_t len;
  uint32_t nameoff;
} fdt_prop_data_t;

uint32_t be32_to_le32(uint32_t be32);
void print_indent(int depth, int is_end);
void traverse_device_tree(void *dtb_ptr);
void dtb_find_initramfs_start(void *dtb_ptr);

#endif