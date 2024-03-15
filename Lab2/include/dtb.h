#include <stdint.h>

#ifndef DTB_H
#define DTB_H
/**********************************************************
 * Convert the Big endian to little endian
 **********************************************************/
typedef union {
  uint32_t u32;
  unsigned char u8[4];
} u_32;

typedef union {
  uint64_t u64;
  unsigned char u8[8];
} u_64;

// Implement two convert functions.
int32_t b2l_32(int32_t);
int64_t b2l_64(int64_t);

/*****************************
 * Flattened Devicetree Header (5.2)
 *****************************/
typedef struct {
  uint32_t magic;          // Should be 0xd00dfeed (big-endian)
  uint32_t totalsize;      // Total size of whole dts in bytes.
  uint32_t off_dt_struct;  // Offset from begin of header to struct (in bytes).
  uint32_t off_dt_strings; // Offset to string block
  uint32_t off_mem_rsvmap; // Offset to memory reserve blocks
  uint32_t version;        // Version of dts
  uint32_t last_comp_version; // the compiler version
  uint32_t
      boot_cpuid_phys; // The id of CPU, as same as reg property in CPU field.
  uint32_t size_dt_strings; // Size of string block in dtb.
  uint32_t size_dt_struct;  // Size of structure block in dtb.
} fdt_header;

/******************************************************************************
 * Reserved region describe in fdt, which contain a list of pair of
 * address and size. The region should not touch by Applications.
 *
 * Note: if address == 0&& size == 0
 * 	 then entry this is the last record.
 * *****************************************************************************/
typedef struct {
  uint64_t address;
  uint64_t size;
} fdt_reserve_entry;

/******************************************************************************
 * Structure Block:
 * ***************************************************************************/

/******************************************************************************
 * Lexical Block: each beginning with a 32-bit Big endian integer. Some of them
 * may follow by some extradata. The whole structure will be padding to 32 bits. (5.4.1)
 * *****************************************************************************/
// Begin of node.
// Contains the unit's name as extra data.
// The name is a null terminate string. And may padding at the end.
// Usually follewed by FDT_END_NODE
#define FDT_BEGIN_NODE 0x00000001

// End of node's expression
// No extra data
// Usually followed by any other token EXCEPT FDT_PROP
#define FDT_END_NODE 0x00000002

// The begin of the representation of one property in DTS
// Must has extra data.
// The property's value is given as byte string and NULL terminate(len).
// 	May Zero padding.
// Usually the next token will be and EXCEPT FDT_END
#define FDT_PROP 0x00000003
typedef struct {
  uint32_t len;     // The lenth of the following value
  uint32_t nameoff; // property's name's offset from strings block
                    // In Null-terminate string.
} fdt_prop;

// Ignore this token
// No extra data
// The next token can be anything.
#define FDT_NOP 0x00000004

// Marks the end of the structure block. Only ONE FDT_END in structure block.
// No extra data
// Can check the byte just after FDT_END == size_dt_struct
#define FDT_END 0x00000009

/*****************************************************************************
 * String Blocks: Null-terminated strings are concatenated together.
 * No aligment.
 * **************************************************************************/

/*****************************************************************************
 * Alignment:
 * 1. Memory reserved block need to alignment in 8-bytes.
 * 2. Structure block alignment in 4-bytes.
 *
 *****************************************************************************/

// Dump all fdt information.
int fdt_dump(void *);

// Traverse the decent information.
// 1: Start address.
// 2: the name of constant
int fdt_traverse(void *, const char *);

// Find attribute and callback
// 1: Start address of FDT
// 2: Node name
// 3: Property name
// 4: Callback function
int fdt_find_do(void *, const char *, int (*fn)(void *, int));

// Target properties location of initrd.
// "linux,initrd-start"
// "linux,initrd-end"
int dump_fdt_header(fdt_header *dtb_info);

#endif // DTB_H