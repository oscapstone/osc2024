#include "include/dtb.h"
#include "include/buddy_system.h"
#include "include/cpio.h"
#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"
#include "include/utils.h"

extern cpio_newc_header_t *cpio_header;
extern char *cpio_end;
extern char *dtb_ptr;

uint32_t dtb_be32_to_le32(uint32_t be32) {
  const uint8_t *bytes = (const uint8_t *)&be32;
  return ((uint32_t)bytes[3] << 0) | ((uint32_t)bytes[2] << 8) |
         ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[0] << 24);
}

uint64_t dtb_be64_to_le64(uint64_t be64) {
  const uint8_t *bytes = (const uint8_t *)&be64;
  return ((uint64_t)bytes[7] << 0) | ((uint64_t)bytes[6] << 8) |
         ((uint64_t)bytes[5] << 16) | ((uint64_t)bytes[4] << 24) |
         ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[2] << 40) |
         ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[0] << 56);
}

void dtb_print_indent(int depth, int is_end) {
  for (int i = 0; i < depth - 1; ++i) {
    uart_sendline("|   ");
  }
  if (!is_end && depth > 0) {
    uart_sendline("|-- ");
  }
}

void dtb_traverse_device_tree() {
  fdt_header_t *hdr = (fdt_header_t *)dtb_ptr;
  if (dtb_be32_to_le32(hdr->magic) != FDT_MAGIC) {
    uart_sendline("Invalid device tree blob header.\n");
    return;
  }
  char *struct_start = (char *)dtb_ptr + dtb_be32_to_le32(hdr->off_dt_struct);
  char *struct_end =
      (char *)struct_start + dtb_be32_to_le32(hdr->size_dt_struct);
  char *strings_ptr = (char *)dtb_ptr + dtb_be32_to_le32(hdr->off_dt_strings);
  char *p = struct_start;
  int depth = 0;
  uart_sendline("root node");
  int count = 0;
  while (p < struct_end) {
    if (++count % 200 == 0)
      uart_async_getc();
    uint32_t token = dtb_be32_to_le32(*(uint32_t *)p);
    p += 4;
    switch (token) {
    case FDT_BEGIN_NODE: {
      char *name = p;
      p += (strlen(name) + 4) & ~3;
      dtb_print_indent(depth++, 0);
      uart_sendline("%s {\n", name);
      break;
    }
    case FDT_END_NODE:
      dtb_print_indent(depth--, 1);
      uart_sendline("}\n");
      break;
    case FDT_PROP: {
      fdt_prop_data_t *prop_data = (fdt_prop_data_t *)p;
      uint32_t len = dtb_be32_to_le32(prop_data->len);
      uint32_t nameoff = dtb_be32_to_le32(prop_data->nameoff);
      char *name = strings_ptr + nameoff;
      unsigned char *value = (unsigned char *)(prop_data + 1);
      dtb_print_indent(depth, 0);
      uart_sendline("%s = 0x", name);
      for (int i = 0; i < len; ++i) {
        uart_sendline("%x", (value[i] >> 4) & 0xF);
        uart_sendline("%x", value[i] & 0xF);
      }
      uart_sendline("\n");
      p += sizeof(fdt_prop_data_t) + ((len + 3) & ~3);
      break;
    }
    case FDT_NOP:
      break;
    case FDT_END:
      uart_sendline("End of device tree.\n");
      return;
    default:
      uart_sendline("Unknown token\n");
      return;
    }
  }
}

void dtb_initramfs_init() {
  fdt_header_t *hdr = (fdt_header_t *)dtb_ptr;
  if (dtb_be32_to_le32(hdr->magic) != FDT_MAGIC) {
    uart_sendline("Invalid device tree blob header.\n");
    return;
  }
  char *struct_start = (char *)dtb_ptr + dtb_be32_to_le32(hdr->off_dt_struct);
  char *struct_end =
      (char *)struct_start + dtb_be32_to_le32(hdr->size_dt_struct);
  char *strings_ptr = (char *)dtb_ptr + dtb_be32_to_le32(hdr->off_dt_strings);
  char *p = struct_start;
  while (p < struct_end) {
    uint32_t token = dtb_be32_to_le32(*(uint32_t *)p);
    p += 4;
    if (token == FDT_BEGIN_NODE) {
      char *name = p;
      p += (strlen(name) + 4) & ~3;
    } else if (token == FDT_PROP) {
      fdt_prop_data_t *prop_data = (fdt_prop_data_t *)p;
      uint32_t len = dtb_be32_to_le32(prop_data->len);
      uint32_t nameoff = dtb_be32_to_le32(prop_data->nameoff);
      char *name = strings_ptr + nameoff;

      if (strcmp(name, "linux,initrd-start") == 0) {
        cpio_header = (cpio_newc_header_t *)(unsigned long)dtb_be32_to_le32(
            *(uint32_t *)(prop_data + 1));
        uart_sendline("Initramfs start found at address: 0x%p.\n",
                      (unsigned long)cpio_header);
        uart_sendline("Initramfs end found at address: 0x%p.\n",
                      (unsigned long)cpio_end);
        return;
      } else if (strcmp(name, "linux,initrd-end") == 0) {
        cpio_end = (char *)(unsigned long)dtb_be32_to_le32(
            *(uint32_t *)(prop_data + 1));
      }
      p += sizeof(fdt_prop_data_t) + ((len + 3) & ~3);
    } else if (token == FDT_END) {
      break;
    }
  }
  uart_sendline("Initramfs start address not found.\n");
}

void dtb_reserve_memory() {
  fdt_header_t *hdr = (fdt_header_t *)dtb_ptr;
  if (dtb_be32_to_le32(hdr->magic) != FDT_MAGIC) {
    uart_sendline("Invalid device tree blob header.\n");
    return;
  }
  char *mem_rsvmap_ptr =
      (char *)dtb_ptr + dtb_be32_to_le32(hdr->off_mem_rsvmap);
  fdt_reserve_entry_t *rsv_entry = (fdt_reserve_entry_t *)mem_rsvmap_ptr;
  while (dtb_be64_to_le64(rsv_entry->address) != 0 ||
         dtb_be64_to_le64(rsv_entry->size) != 0) {
    uint64_t start = dtb_be64_to_le64(rsv_entry->address);
    uint64_t size = dtb_be64_to_le64(rsv_entry->size);
    uint64_t end = start + size;
    uart_sendline("[Startup Allocation] Reserving memory for device tree "
                  "specified memory from 0x%p "
                  "to 0x%p.\n",
                  start, end);
    startup_memory_block_table_add(start, end);
    rsv_entry++;
  }
  uint64_t dtb_start = (uint64_t)dtb_ptr;
  uint64_t dtb_end = dtb_start + dtb_be32_to_le32(hdr->totalsize);
  uart_sendline("[Startup Allocation] Reserving memory for device tree itself "
                "from 0x%p to 0x%p.\n",
                dtb_start, dtb_end);
  startup_memory_block_table_add(dtb_start, dtb_end);
}