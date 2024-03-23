#include "devicetree.hpp"

#include "board/mini-uart.hpp"
#include "string.hpp"

DeviceTree dt;

bool DeviceTree::init(void* addr) {
  base = (char*)addr;

  mini_uart_printf("DTB ADDR: %p %lx\n", base, *(long*)base);

  header = (fdt_header*)addr;
  if (!header->valid()) {
    mini_uart_printf("invalid dtb header 0x%x != 0x%x\n", header->magic(),
                     header->MAGIC);
    return false;
  }
  if (header->last_comp_version() > LAST_COMP_VERSION) {
    mini_uart_printf("last compatible version %d > %d\n", header->version(),
                     LAST_COMP_VERSION);
    return false;
  }

  mini_uart_printf("magic             %x\n", header->magic());
  mini_uart_printf("totalsize         %x\n", header->totalsize());
  mini_uart_printf("off_dt_struct     %x\n", header->off_dt_struct());
  mini_uart_printf("off_dt_strings    %x\n", header->off_dt_strings());
  mini_uart_printf("off_mem_rsvmap    %x\n", header->off_mem_rsvmap());
  mini_uart_printf("version           %x\n", header->version());
  mini_uart_printf("last_comp_version %x\n", header->last_comp_version());
  mini_uart_printf("boot_cpuid_phys   %x\n", header->boot_cpuid_phys());
  mini_uart_printf("size_dt_strings   %x\n", header->size_dt_strings());
  mini_uart_printf("size_dt_struct    %x\n", header->size_dt_struct());

  reserve_entry = (fdt_reserve_entry*)(base + header->off_mem_rsvmap());
  for (auto it = reserve_entry; it->address() and it->size(); it++) {
    mini_uart_printf("rsvmap %ld: %lx %lx\n", it - reserve_entry, it->address(),
                     it->size());
  }

  auto tree_base = (uint32_t*)(base + header->off_dt_struct());
  auto str_base = (char*)(base + header->off_dt_strings());
  for (int i = 0, sz = header->size_dt_struct(); i < sz;) {
    switch (__builtin_bswap32(tree_base[i++])) {
      case FDT_BEGIN_NODE: {
        auto name = (char*)&tree_base[i];
        auto len = strlen(name);
        i += (len + 4) / (sizeof(uint32_t) / sizeof(char));

        mini_uart_printf("begin node %d %s\n", len, name);
        break;
      }
      case FDT_END_NODE:
        mini_uart_printf("end node\n");
        break;
      case FDT_PROP: {
        auto len = __builtin_bswap32(tree_base[i++]);
        auto nameoff = __builtin_bswap32(tree_base[i++]);
        auto prop = (char*)&tree_base[i];
        i += (len + 3) / (sizeof(uint32_t) / sizeof(char));
        mini_uart_printf("\tprop %s: ", &str_base[nameoff]);
        for (uint32_t j = 0; j < len; j++)
          mini_uart_putc(prop[j]);
        mini_uart_putc('\n');
        break;
      }
      case FDT_NOP:
        break;
      case FDT_END:
        i = sz;
        break;
    }
  }

  return true;
}
