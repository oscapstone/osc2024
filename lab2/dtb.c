#include "dtb.h"
#include "include/cpio.h"
#include "uart.h"
#include "utils.h"

extern cpio_newc_header *cpio_header;
char *dtb_ptr;

uint32_t be32_to_le32(uint32_t be32) {
  const uint8_t *bytes = (const uint8_t *)&be32;
  return ((uint32_t)bytes[3] << 0) | ((uint32_t)bytes[2] << 8) |
         ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[0] << 24);
}

void print_indent(int depth, int is_end) {
  for (int i = 0; i < depth - 1; ++i) {
    uart_send("|   ");
  }
  if (!is_end && depth > 0) {
    uart_send("|-- ");
  }
}

void traverse_device_tree(void *dtb_ptr) {
  fdt_header *hdr = (fdt_header *)dtb_ptr;
  if (be32_to_le32(hdr->magic) != FDT_MAGIC) {
    uart_send("Invalid device tree blob header!\n");
    return;
  }
  char *struct_start = (char *)dtb_ptr + be32_to_le32(hdr->off_dt_struct);
  char *struct_end = (char *)struct_start + be32_to_le32(hdr->size_dt_struct);
  char *strings_ptr = (char *)dtb_ptr + be32_to_le32(hdr->off_dt_strings);
  char *p = struct_start;
  int depth = 0;

  uart_send("root node");
  int count = 0;
  while (p < struct_end) {
    if (++count % 200 == 0)
      uart_getc();
    uint32_t token = be32_to_le32(*(uint32_t *)p);
    p += 4;
    switch (token) {
    case FDT_BEGIN_NODE: {
      char *name = p;
      p += (strlen(name) + 4) & ~3;
      print_indent(depth++, 0);
      uart_send("%s {\n", name);
      break;
    }
    case FDT_END_NODE:
      print_indent(depth--, 1);
      uart_send("}\n");
      break;
    case FDT_PROP: {
      fdt_prop_data_t *prop_data = (fdt_prop_data_t *)p;
      uint32_t len = be32_to_le32(prop_data->len);
      uint32_t nameoff = be32_to_le32(prop_data->nameoff);
      char *name = strings_ptr + nameoff;
      unsigned char *value = (unsigned char *)(prop_data + 1);
      print_indent(depth, 0);
      uart_send("%s = 0x", name);
      for (int i = 0; i < len; ++i) {
        uart_send("%x", (value[i] >> 4) & 0xF);
        uart_send("%x", value[i] & 0xF);
      }
      uart_send("\n");
      p += sizeof(fdt_prop_data_t) + ((len + 3) & ~3);
      break;
    }
    case FDT_NOP:
      break;
    case FDT_END:
      uart_send("End of device tree\n");
      return;
    default:
      uart_send("Unknown token\n");
      return;
    }
  }
}

void dtb_find_initramfs_start(void *dtb_ptr) {
  fdt_header *hdr = (fdt_header *)dtb_ptr;
  if (be32_to_le32(hdr->magic) != FDT_MAGIC) {
    uart_send("Invalid device tree blob header!\n");
    return;
  }
  char *struct_start = (char *)dtb_ptr + be32_to_le32(hdr->off_dt_struct);
  char *struct_end = (char *)struct_start + be32_to_le32(hdr->size_dt_struct);
  char *strings_ptr = (char *)dtb_ptr + be32_to_le32(hdr->off_dt_strings);
  char *p = struct_start;

  while (p < struct_end) {
    uint32_t token = be32_to_le32(*(uint32_t *)p);
    p += 4;
    if (token == FDT_BEGIN_NODE) {
      char *name = p;
      p += (strlen(name) + 4) & ~3;
    } else if (token == FDT_PROP) {
      fdt_prop_data_t *prop_data = (fdt_prop_data_t *)p;
      uint32_t len = be32_to_le32(prop_data->len);
      uint32_t nameoff = be32_to_le32(prop_data->nameoff);
      char *name = strings_ptr + nameoff;

      if (strcmp(name, "linux,initrd-start") == 0) {
        cpio_header = (cpio_newc_header *)(unsigned long long)be32_to_le32(
            *(uint32_t *)(prop_data + 1));
        uart_send("Initramfs start found at address: 0x%p\n", cpio_header);
        return;
      }
      p += sizeof(fdt_prop_data_t) + ((len + 3) & ~3);
    } else if (token == FDT_END) {
      break;
    }
  }

  uart_send("Initramfs start address not found.\n");
}
