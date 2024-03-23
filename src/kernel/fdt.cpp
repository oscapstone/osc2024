#include "fdt.hpp"

#include "board/mini-uart.hpp"
#include "heap.hpp"
#include "string.hpp"

FDT fdt;

static bool print_fdt(uint32_t tag, int level, const char* node_name,
                      const char* prop_name, uint32_t len,
                      const char prop_value[]) {
  for (int i = 0; i < level; i++)
    mini_uart_putc('\t');
  switch (tag) {
    case FDT_BEGIN_NODE: {
      mini_uart_printf("begin %s", node_name);
      break;
    }
    case FDT_END_NODE: {
      mini_uart_printf("end %s", node_name);
      break;
    }
    case FDT_PROP: {
      mini_uart_printf("%s: ", prop_name);
      bool printable = true;
      for (uint32_t i = 0; i < len; i++) {
        auto x = prop_value[i];
        printable &= (0x20 <= x and x <= 0x7e) or (i + 1 == len and x == 0);
      }
      for (uint32_t j = 0; j < len; j++)
        if (printable)
          mini_uart_putc(prop_value[j]);
        else
          mini_uart_printf("%02x", prop_value[j]);
      break;
    }
    default: {
      mini_uart_printf("unknown tag %x", tag);
    }
  }
  mini_uart_putc('\n');
  return false;
}

bool FDT::init(void* addr) {
  base = (char*)addr;

  mini_uart_printf("DTB ADDR: %p\n", base);

  if (fdt_magic(base) != FDT_MAGIC) {
    mini_uart_printf("invalid dtb header 0x%x != 0x%x\n", fdt_magic(base),
                     FDT_MAGIC);
    return false;
  }
  if (fdt_last_comp_version(base) > LAST_COMP_VERSION) {
    mini_uart_printf("Unsupport dtb v%d > v%d\n", fdt_last_comp_version(base),
                     LAST_COMP_VERSION);
    return false;
  }

  mini_uart_printf("magic             %x\n", fdt_magic(base));
  mini_uart_printf("totalsize         %x\n", fdt_totalsize(base));
  mini_uart_printf("off_dt_struct     %x\n", fdt_off_dt_struct(base));
  mini_uart_printf("off_dt_strings    %x\n", fdt_off_dt_strings(base));
  mini_uart_printf("off_mem_rsvmap    %x\n", fdt_off_mem_rsvmap(base));
  mini_uart_printf("version           %x\n", fdt_version(base));
  mini_uart_printf("last_comp_version %x\n", fdt_last_comp_version(base));
  mini_uart_printf("boot_cpuid_phys   %x\n", fdt_boot_cpuid_phys(base));
  mini_uart_printf("size_dt_strings   %x\n", fdt_size_dt_strings(base));
  mini_uart_printf("size_dt_struct    %x\n", fdt_size_dt_struct(base));

  reserve_entry = (fdt_reserve_entry*)(base + fdt_off_mem_rsvmap(base));
  for (auto it = reserve_entry; fdtrsv_address(it) and fdtrsv_size(it); it++) {
    mini_uart_printf("rsvmap %ld: %lx %lx\n", it - reserve_entry,
                     fdtrsv_address(it), fdtrsv_size(it));
  }

  traverse(print_fdt);

  return true;
}

void FDT::traverse(fp callback) {
  int level = 0;
  uint32_t offset = 0;
  traverse_impl(level, offset, nullptr, callback);
}

void FDT::traverse_impl(int& level, uint32_t& offset, const char* node_name,
                        fp callback) {
  while (offset < fdt_size_dt_struct(base)) {
    auto tag = fdt_ld(*struct_base(offset));
    offset += 4;
    auto hdr = struct_base(offset);
    switch (tag) {
      case FDT_BEGIN_NODE: {
        auto name = fdtn_name(hdr);
        auto len = strlen(name);
        offset += align<4>(len + 1);
        if (callback(tag, level, name, nullptr, 0, nullptr))
          return;
        level++;
        traverse_impl(level, offset, name, callback);
        break;
      }
      case FDT_END_NODE: {
        level--;
        if (callback(tag, level, node_name, nullptr, 0, nullptr))
          return;
        return;
      }
      case FDT_PROP: {
        auto len = fdtp_len(hdr);
        auto nameoff = fdtp_nameoff(hdr);
        auto prop = fdtp_prop(hdr);
        offset += align<4>(sizeof(fdt_prop) + len);

        auto prop_name = str_base(nameoff);
        if (callback(tag, level, node_name, prop_name, len, prop))
          return;
        break;
      }
      case FDT_NOP:
        break;
      case FDT_END:
        return;
    }
  }
}
