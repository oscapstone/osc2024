#include "fdt.h"

#include "int.h"
#include "string.h"
#include "uart.h"

#define FDT_VERSION 17
#define FDT_MAGIC 0xD00DFEED

/* convert each field in fdt_header to little endian */
fdt_header_t fdt_header2le(const fdt_header_t *be_header) {
  return (fdt_header_t){
      .magic = be2le_32(be_header->magic),
      .totalsize = be2le_32(be_header->totalsize),
      .off_dt_struct = be2le_32(be_header->off_dt_struct),
      .off_dt_strings = be2le_32(be_header->off_dt_strings),
      .off_mem_rsvmap = be2le_32(be_header->off_mem_rsvmap),
      .version = be2le_32(be_header->version),
      .last_comp_version = be2le_32(be_header->last_comp_version),
      .boot_cpuid_phys = be2le_32(be_header->boot_cpuid_phys),
      .size_dt_strings = be2le_32(be_header->size_dt_strings),
      .size_dt_struct = be2le_32(be_header->size_dt_struct),
  };
}

fdt_prop_t fdt_prop2le(const fdt_prop_t *be_prop) {
  return (fdt_prop_t){
      .nameoff = be2le_32(be_prop->nameoff),
      .len = be2le_32(be_prop->len),
  };
}

int fdt_traversal(char *dbt, traversal_callback_t cb) {
  uart_printf("dbt loaded at: %x\n", (size_t)dbt);

  fdt_header_t header = fdt_header2le((fdt_header_t *)dbt);

  if (header.magic != FDT_MAGIC) {
    uart_printf("Invalid fdt magic value: %x\n", header.magic);
    return -1;
  }

  if (header.last_comp_version > FDT_VERSION) {
    uart_printf(
        "Last compatible version(%d) is higher than your fdt version(%d)\n",
        header.last_comp_version, FDT_VERSION);
    return -2;
  }

  char *cur = dbt + header.off_dt_struct;

  char *struct_end = cur + header.size_dt_struct;
  char *strings_ptr = dbt + header.off_dt_strings;

  while (cur < struct_end) {
    u32_t token = be2le_32(*(u32_t *)cur);
    cur += sizeof(u32_t);

    switch (token) {
      case FDT_BEGIN_NODE: {
        char *name = cur;
        cb((fdt_node_t){
            .tag = token,
            .begin_node = {.node_name = name},
        });

        size_t size = strnlen(name, 256);
        cur += align(size, 4);
        break;
      }

      case FDT_END_NODE: {
        cb((fdt_node_t){.tag = token, .end_node = {}});
        break;
      }

      case FDT_PROP: {
        fdt_prop_t *be_prop = (fdt_prop_t *)cur;
        fdt_prop_t prop = fdt_prop2le(be_prop);

        cur += sizeof(fdt_prop_t);

        char *property_name = strings_ptr + prop.nameoff;
        cb((fdt_node_t){
            .tag = token,
            .prop = {.prop_name = property_name, .prop = prop, .prop_val = cur},
        });

        cur += align(prop.len, 4);
        break;
      }

      case FDT_NOP: {
        cb((fdt_node_t){.tag = token, .nop = {}});
        break;
      }

      case FDT_END: {
        cb((fdt_node_t){.tag = token, .end = {}});
        return 0;
      }

      default: {
        break;
      }
    }
  }

  return 0;
}
