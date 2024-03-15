#include "dtb.h"
#include "uart.h"
#include "mystring.h"
#include <stdint.h>

/**********************************************************
 * Support functions
 *********************************************************/

// Convert 32 bit Big endian -> little endian.
int32_t b2l_32(int32_t input) {
  u_32 t;
  unsigned char tmp;
  t.u32 = input;
  for (int i = 0; i < 2; i++) {
    tmp = t.u8[i];
    t.u8[i] = t.u8[3 - i];
    t.u8[3 - i] = tmp;
  }
  return t.u32;
}

// Convert 64 bit Big endian -> little endian.
int64_t b2l_64(int64_t input) {
  u_64 t;
  unsigned char tmp;
  t.u64 = input;
  // Swap
  for (int i = 0; i < 4; i++) {
    tmp = t.u8[i];
    t.u8[i] = t.u8[7 - i];
    t.u8[7 - i] = tmp;
  }
  return t.u64;
}

/************************************************
 * Dump the fdt_header
 ***********************************************/
int dump_fdt_header(fdt_header *dtb_info) {
  // Magic
  uart_printf("Magic:  %x\n", dtb_info->magic);
//   uart_puts("Magic: ");
//   uart_puth(dtb_info->magic);
//   uart_puts("\n");
  // Total size
  uart_printf("Total Size: %x\n", dtb_info->totalsize);
//   uart_puts("Total Size: ");
//   uart_puth(dtb_info->totalsize);
//   uart_puts("\n");
  // Offset of struct block
  uart_printf("Struct Offset: %x\n", dtb_info->off_dt_struct);
//   uart_puts("Struct Offset: ");
//   uart_puth(dtb_info->off_dt_struct);
//   uart_puts("\n");
  // Offset of string block
  uart_printf("Struct Offset: %x\n", dtb_info->off_dt_strings);
//   uart_puts("String Offset: ");
//   uart_puth(dtb_info->off_dt_strings);
//   uart_puts("\n");
  // Offset of memory reserve map
  uart_printf("MemRsvMap Offset: %x\n", dtb_info->off_mem_rsvmap);
//   uart_puts("MemRsvMap Offset: ");
//   uart_puth(dtb_info->off_mem_rsvmap);
//   uart_puts("\n");
  // Version
  uart_printf("Version: %x\n", dtb_info->version);
//   uart_puts("Version: ");
//   uart_puth(dtb_info->version);
//   uart_puts("\n");
  // Compile version
  uart_printf("Compiler version: %x\n", dtb_info->last_comp_version);
//   uart_puts("Compiler version: ");
//   uart_puth(dtb_info->last_comp_version);
//   uart_puts("\n");
  // The id of cpu which will boot the device.
  uart_printf("CPU ID: %x\n", dtb_info->boot_cpuid_phys);
//   uart_puts("CPU ID: ");
//   uart_puth(dtb_info->boot_cpuid_phys);
//   uart_puts("\n");
  // Size of strng block.
  uart_printf("String block size: %x\n", dtb_info->size_dt_strings);
//   uart_puts("String block size: ");
//   uart_puth(dtb_info->size_dt_strings);
//   uart_puts("\n");
  // Size of struct block
  uart_printf("Struct block size: %x\n", dtb_info->size_dt_struct);
//   uart_puts("Struct block size: ");
//   uart_puth(dtb_info->size_dt_struct);
//   uart_puts("\n");
  return 0;
}

/*************************************************
 * Convert the ftd_header to little endian
 ************************************************/
static int fdt_header_b2l(fdt_header *dtb_info) {
  // If the header is already little endian, ignore it.
  if (dtb_info->magic == 0xd00dfeed)
    return 0;
    
  dtb_info->magic = b2l_32(dtb_info->magic);
  dtb_info->totalsize = b2l_32(dtb_info->totalsize);
  dtb_info->off_dt_struct = b2l_32(dtb_info->off_dt_struct);
  dtb_info->off_dt_strings = b2l_32(dtb_info->off_dt_strings);
  dtb_info->off_mem_rsvmap = b2l_32(dtb_info->off_mem_rsvmap);
  dtb_info->version = b2l_32(dtb_info->version);
  dtb_info->last_comp_version = b2l_32(dtb_info->last_comp_version);
  dtb_info->boot_cpuid_phys = b2l_32(dtb_info->boot_cpuid_phys);
  dtb_info->size_dt_struct = b2l_32(dtb_info->size_dt_struct);
  dtb_info->size_dt_strings = b2l_32(dtb_info->size_dt_strings);
  return 0;
}

/**************************************************************
 * dtb_start: The start address of FDT
 * off_struct: The offset of struct block
 * off_string: The offset of string block
 * size_struct: The size of struct block
 **************************************************************/
static int dump_fdt_struct(char *dtb_start, uint32_t off_struct,
                           uint32_t off_string, uint32_t size_struct) {
  // calculate the start of struct block
  char *cur = dtb_start + off_struct;
  char *str_lo = dtb_start + off_string;
  uint32_t *tag;   // Struct tag
  uint32_t t_int;  // tmp tag
  fdt_prop *prop;  // property tag
  fdt_prop t_prop; // tmp property
  int pad = 0;

  while (1) {
    tag = (uint32_t *)cur;
    t_int = b2l_32(*tag);
    // uart_puth(cur);
    switch (t_int) {
    // begin node
    case FDT_BEGIN_NODE:
      uart_puts("Node name: ");
      cur += 4; // skip FDT_BEGIN tag
      uart_puts(cur);
      uart_puts("\n");
      pad = 0;
      // IF no name, Add 4-bytes.
      if (*cur == 0) {
        cur += 4;
        break;
      }
      // to NULL
      while (*cur) {
        cur++;
        pad++;
      }
      cur += 1;
      pad += 1;
      // pad to align
      pad = (4 - (pad % 4)) % 4;
      cur += pad;
      break;
      // end node
    case FDT_END_NODE:
      uart_puts("END_NODE\n");
      cur += 4;
      break;

      // property node
    case FDT_PROP:
      cur += 4;
      prop = (fdt_prop *)cur;
      t_prop.nameoff = b2l_32(prop->nameoff);
      t_prop.len = b2l_32(prop->len);
      uart_puts("Property: ");
      uart_puts(str_lo + t_prop.nameoff);
      uart_puts("\n");
      cur += 8;
      uart_puts("PROP_LEN: ");
      uart_printf("%d",t_prop.len);
      uart_puts("\n=====================\n");
      if (t_prop.len == 0) {
        break;
      }
    //   uart_putsn(cur, t_prop.len); // Not all value is string
    //   uart_puts("\n");
      cur += t_prop.len;
      cur += (4 - (t_prop.len % 4)) % 4; // Padding to 4.
      break;

      // END
    case FDT_END:
      return 0;
      break;

    case FDT_NOP:
      cur += 4;
      break;

    default:
      //uart_puts("Error!\n");
      //uart_puth((int *)cur);
      cur += 4;
    }
  }
  return 0;
}

/****************************************************************
 * FDT traverse
 ***************************************************************/

int fdt_dump(void *dtb_start) {
  // Get the header information.
  fdt_header *dtb_info = (fdt_header *)dtb_start;

  fdt_header_b2l(dtb_info);

  // Dump metadata info
  dump_fdt_header(dtb_info);

  // Dump Memory reserve map
  // TODO: implement
  // dump_fdt_rsvmap((char*) dtb_start, dtb_info->off_mem_rsvmap)

  // Dump Structure block
  // struct fdt_header -> memory reservation block -> structure block -> strings block

  dump_fdt_struct((char *)dtb_start, dtb_info->off_dt_struct,
                  dtb_info->off_dt_strings, dtb_info->size_dt_struct);

  return 0;
}


// 1: Start address of FDT
// 2: Node name
// 3: Property name
// 4: Callback function

int fdt_find_do(void *dtb_start, const char *name, int (*fn)(void *, int)) {
  char *cur = (char *)dtb_start;
  uint32_t off_struct;
  uint32_t off_string;
  uint32_t size_struct;

  // Get the header information.
  fdt_header *dtb_info = (fdt_header *)dtb_start;

  // Big to little endian
  fdt_header_b2l(dtb_info);

  off_struct = dtb_info->off_dt_struct;
  off_string = dtb_info->off_dt_strings;
  size_struct = dtb_info->size_dt_struct;

  // Traverse
  char *str_lo = cur + off_string;
  cur = cur + off_struct;
  char *str_tmplo = cur;
  uint32_t *tag;   // Struct tag
  uint32_t t_tag;  // tmp tag, don't change the content in dtb
  fdt_prop *prop;  // property tag
  fdt_prop t_prop; // tmp prop
  int pad = 0;

  while (1) {
    tag = (uint32_t *)cur;
    t_tag = b2l_32(*tag);
    switch (t_tag) {
    // begin node
    case FDT_BEGIN_NODE:
      cur += 4;
      str_tmplo = cur;
      pad = 0;
      if (*cur == 0) { // IF no name, Add 4-bytes.
        cur += 4;
        break;
      }
      while (*cur) {
        cur++;
        pad++;
      }
      cur += 1; // End of string ('\0');
      pad += 1;
      pad = (4 - (pad % 4)) % 4; // Padding to 4-byte
      cur += pad;
      break;
      // end node
    case FDT_END_NODE:
      cur += 4; // Jump to next record
      break;

      // property node
    case FDT_PROP:
      cur += 4;
      prop = (fdt_prop *)cur;
      // name offset from string block
      t_prop.nameoff = b2l_32(prop->nameoff);
      t_prop.len = b2l_32(prop->len);
      cur += 8;
      if (!strcmp(str_lo + t_prop.nameoff, name)) {
        uart_puts("\n=====================\n");
        uart_puts("Node name: ");
        uart_puts(str_tmplo);
        uart_puts("\n");
        uart_puts("Property: ");
        uart_puts(str_lo + t_prop.nameoff);
        uart_puts("\n");
        uart_puts("PROP_LEN: ");
        uart_printf("%d",t_prop.len);
        uart_puts("\n=====================\n");
        fn((void *)cur, t_prop.len);
        return 0;
      }
      if (prop->len == 0) {
        break;
      }
      cur += t_prop.len;
      cur += (4 - (t_prop.len % 4)) % 4; // Padding to 4.
      break;

      // END
    case FDT_END:
      return 0;
      break;

    case FDT_NOP:
      cur += 4; // Ignore
      break;

    default:
      cur += 4; // Error!
    }
  }
  uart_puts(name);
  uart_puts(" Cannot Find!!\n");
  return 1;
}