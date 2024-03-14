#ifndef __DEVICETREE_H__
#define __DEVICETREE_H__

#include <stdint.h>

// all field store in big-endian
typedef struct{
  uint32_t magic; // 0xd00dfeed (big-endian) (edfe0dd0)?
  uint32_t totalsize; // including mem_reserved_blk, struct_blk, string_blk, space gap.
  uint32_t off_dt_struct; // offset of the structure block from begining of header
  uint32_t off_dt_strings; // offset of the string block from begining of header
  uint32_t off_mem_rsvmap; // offset of the memory reservation block from begining of header
  uint32_t version; // version of the devicetree data structure: 17
  uint32_t last_comp_version; // 16
  uint32_t boot_cpuid_phys; // physical boot cpu id
  uint32_t size_dt_strings; // string block size
  uint32_t size_dt_struct; // structure block size
}fdt_header;


/* for memory reservation block , the last block address and size should be 0*/
typedef struct{
  uint64_t address;
  uint64_t size;
}fdt_reserve_entry;


/* for structure block */
/* all token is stored as big-endian 32-bit integer */
/* all token shall be aligned 4 bytes */

/* followed by node's name */
/* next token can't be FDT_END */
#define FDT_BEGIN_NODE 0x00000001

/* next token can't be FDT_PROP */
#define FDT_END_NODE 0x00000002

/* followed by property value, stored in big-endian */
/* next token can't be FDT_END */
#define FDT_PROP 0x00000003
typedef struct{
  uint32_t len;
  uint32_t nameoff; // property name's offset from the beginning of string block
}fdt_prop;

#define FDT_NOP 0x00000004

/* There shall be only one FDT_END */
/* followed by the offset from beginning of structure block = size_dt_struct */
#define FDT_END 0x00000009


/* string block don't need to align */

/* the memory reservation block shall be aligned to an 8-byte boundary
and the structure block to a 4-byte boundary */

uint32_t swap32(uint32_t n);
void parse_dtb(void *dtb);
void fdt_traverse(void *dtb, void (*callback)(void *, char *));

#endif