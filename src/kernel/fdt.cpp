#include "fdt.hpp"

#include "board/mini-uart.hpp"
#include "string.hpp"

FDT fdt;

bool print_fdt(uint32_t tag, int level, const char* node_name,
               const char* prop_name, uint32_t len, const char prop_value[]) {
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
      mini_uart_print({prop_value, (int)len});
      break;
    }
    default: {
      mini_uart_printf("unknown tag %x", tag);
    }
  }
  mini_uart_putc('\n');
  return false;
}

void FDT::print() {
  traverse(print_fdt);
}

void FDT::init(void* addr, bool debug) {
  base = (char*)addr;

  if (debug)
    mini_uart_printf("DTB ADDR: %p\n", base);

  if (fdt_magic(base) != FDT_MAGIC) {
    mini_uart_printf("invalid dtb header 0x%x != 0x%x\n", fdt_magic(base),
                     FDT_MAGIC);
    prog_hang();
  }
  if (fdt_last_comp_version(base) > LAST_COMP_VERSION) {
    mini_uart_printf("Unsupport dtb v%d > v%d\n", fdt_last_comp_version(base),
                     LAST_COMP_VERSION);
    prog_hang();
  }

  if (debug) {
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
  }

  reserve_entry = (fdt_reserve_entry*)(base + fdt_off_mem_rsvmap(base));
  if (debug) {
    for (auto it = reserve_entry; fdtrsv_address(it) and fdtrsv_size(it);
         it++) {
      mini_uart_printf("rsvmap %ld: %lx %lx\n", it - reserve_entry,
                       fdtrsv_address(it), fdtrsv_size(it));
    }
  }

  if (debug)
    print();
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

namespace fdt_find {
static const char* path;
static int cur_level, cur_idx, nxt_idx, depth;
static bool last, found, debug;
static string_view view;
static FDT::fp list_fp;
static bool find_path(uint32_t tag, int level, const char* node_name,
                      const char* prop_name, uint32_t len,
                      const char prop_value[]) {
  if (found) {
    if (level < cur_level)
      return true;
    if (!list_fp)
      return true;
    if (depth and level > cur_level + depth)
      return false;
    return list_fp(tag, level - cur_level, node_name, prop_name, len,
                   prop_value);
  }
  if (level != cur_level)
    return false;

  switch (tag) {
    case FDT_BEGIN_NODE: {
      if (debug)
        mini_uart_printf("+ %s\n", node_name);

      int sz = nxt_idx - cur_idx;
      if (strncmp(path + cur_idx, node_name, sz))
        return false;
      if (node_name[sz] != '\0')
        return false;

      if (last) {
        found = true;
        if (list_fp)
          return list_fp(tag, 0, node_name, prop_name, len, prop_value);
        return true;
      } else {
        cur_level++;
        cur_idx = ++nxt_idx;
        for (;;) {
          switch (path[nxt_idx]) {
            case 0:
              last = true;
            case '/':
              return false;
              break;
            default:
              nxt_idx++;
          }
        }
      }
      break;
    }
    case FDT_PROP: {
      if (last) {
        if (debug)
          mini_uart_printf(": %s\n", prop_name);

        if (strncmp(path + cur_idx, prop_name, nxt_idx - cur_idx))
          return false;
        cur_level++;
        view = {prop_value, (int)len};
        return found = true;
      }
    }
  }
  return false;
}
}  // namespace fdt_find

pair<bool, string_view> FDT::find(const char* path, fp list_fp, int depth,
                                  bool debug) {
  using namespace fdt_find;
  fdt_find::path = path;
  fdt_find::list_fp = list_fp;
  fdt_find::depth = depth;
  fdt_find::debug = debug;
  cur_level = 0;
  cur_idx = -1;
  nxt_idx = 0;
  view = {nullptr, 0};
  last = path[1] == '\0';
  found = false;
  if (path[0] == '/')
    traverse(find_path);
  return {found, view};
}
