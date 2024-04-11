#ifndef _DTB_H
#define _DTB_H

#include "lib/int.h"

typedef enum fdt_tag_e {
  FDT_BEGIN_NODE = 0x00000001,
  FDT_END_NODE = 0x00000002,
  FDT_PROP = 0x00000003,
  FDT_NOP = 0x00000004,
  FDT_END = 0x00000009,
} fdt_tag_t;

typedef struct fdt_header_s {
  /* This field shall contain the value 0xd00dfeed */
  u32_t magic;

  /* Total size in bytes of the devicetree data structure*/
  u32_t totalsize;

  /* Offset in bytes of the structure block from the beginning of the header */
  u32_t off_dt_struct;

  /* Offset in bytes of the strings block from the beginning of the header */
  u32_t off_dt_strings;

  /* Offset in bytes of the memory reservation block from the beginning of the
   * header */
  u32_t off_mem_rsvmap;

  /* The version of the devicetree data strcuture */
  u32_t version;

  /* The lowest version of the devicetree data structure with which the version
   * used is backwards compatible. */
  u32_t last_comp_version;

  /* Physical ID of the system's boot CPU. */
  u32_t boot_cpuid_phys;

  /* The length in bytes of the strings block section */
  u32_t size_dt_strings;

  /* The length in bytes of the structure block section */
  u32_t size_dt_struct;
} fdt_header_t;

typedef struct fdt_reserve_entry_s {
  u64_t address;
  u64_t size;
} fdt_reserve_entry_t;

typedef struct fdt_prop_s {
  u32_t len;
  u32_t nameoff;
} fdt_prop_t;

typedef struct fdt_node_u {
  fdt_tag_t tag;

  union {
    struct {
      char *node_name;
    } begin_node;

    struct {
      char *prop_name;
      fdt_prop_t prop;
      char *prop_val;
    } prop;

    struct {
    } end_node, nop, end;
  };

} fdt_node_t;

typedef int (*traversal_callback_t)(fdt_node_t node);

int fdt_traversal(char *dbt, traversal_callback_t cb);

#endif  // _DTB_H
