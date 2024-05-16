#include "fdt.hpp"

#include "io.hpp"
#include "string.hpp"

FDT fdt;

bool print_fdt(uint32_t tag, int level, const char* node_name,
               const char* prop_name, uint32_t len, const char prop_value[]) {
  for (int i = 0; i < level; i++)
    kputc('\t');
  switch (tag) {
    case FDT_BEGIN_NODE: {
      kprintf("begin %s", node_name);
      break;
    }
    case FDT_END_NODE: {
      kprintf("end %s", node_name);
      break;
    }
    case FDT_PROP: {
      kprintf("%s: ", prop_name);
      kprint({prop_value, (int)len});
      break;
    }
    default: {
      kprintf("unknown tag %x", tag);
    }
  }
  kputc('\n');
  return false;
}

void FDT::print() {
  traverse(print_fdt);
}

void FDT::init(void* addr, bool debug) {
  base = (char*)addr;

  klog("DTB addr       : %p ~ %p\n", startp(), endp());

  if (fdt_magic(base) != FDT_MAGIC)
    panic("invalid dtb header 0x%x != 0x%x", fdt_magic(base), FDT_MAGIC);
  if (fdt_last_comp_version(base) > LAST_COMP_VERSION)
    panic("Unsupport dtb v%d > v%d", fdt_last_comp_version(base),
          LAST_COMP_VERSION);

  if (debug) {
    kprintf("magic             %x\n", fdt_magic(base));
    kprintf("totalsize         %x\n", fdt_totalsize(base));
    kprintf("off_dt_struct     %x\n", fdt_off_dt_struct(base));
    kprintf("off_dt_strings    %x\n", fdt_off_dt_strings(base));
    kprintf("off_mem_rsvmap    %x\n", fdt_off_mem_rsvmap(base));
    kprintf("version           %x\n", fdt_version(base));
    kprintf("last_comp_version %x\n", fdt_last_comp_version(base));
    kprintf("boot_cpuid_phys   %x\n", fdt_boot_cpuid_phys(base));
    kprintf("size_dt_strings   %x\n", fdt_size_dt_strings(base));
    kprintf("size_dt_struct    %x\n", fdt_size_dt_struct(base));
  }

  reserve_entry = (fdt_reserve_entry*)(base + fdt_off_mem_rsvmap(base));
  if (debug) {
    for (auto it = reserve_entry; fdtrsv_address(it) and fdtrsv_size(it);
         it++) {
      kprintf("rsvmap %ld: %lx %lx\n", it - reserve_entry, fdtrsv_address(it),
              fdtrsv_size(it));
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

bool FDT::traverse_impl(int& level, uint32_t& offset, const char* node_name,
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
          return true;
        level++;
        if (traverse_impl(level, offset, name, callback))
          return true;
        break;
      }
      case FDT_END_NODE: {
        level--;
        if (callback(tag, level, node_name, nullptr, 0, nullptr))
          return true;
        return false;
      }
      case FDT_PROP: {
        auto len = fdtp_len(hdr);
        auto nameoff = fdtp_nameoff(hdr);
        auto prop = fdtp_prop(hdr);
        offset += align<4>(sizeof(fdt_prop) + len);

        auto prop_name = str_base(nameoff);
        if (callback(tag, level, node_name, prop_name, len, prop))
          return true;
        break;
      }
      case FDT_NOP:
        break;
      case FDT_END:
        return true;
    }
  }
  return false;
}

namespace fdt_find {
static const char* path;
static int cur_level, cur_idx, nxt_idx, depth;
static bool last, found, debug;
static string_view view;
static FDT::fp list_fp;
static bool match(const char* name) {
  int sz = nxt_idx - cur_idx;
  return not strncmp(path + cur_idx, name, sz) and
         (name[sz] == '\0' or name[sz] == '@');
}
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
    auto r =
        list_fp(tag, level - cur_level, node_name, prop_name, len, prop_value);
    if (cur_level == level and tag == FDT_END_NODE)
      return true;
    return r;
  }
  if (level != cur_level)
    return false;

  switch (tag) {
    case FDT_BEGIN_NODE: {
      if (debug)
        kprintf("+ %s\n", node_name);

      if (!match(node_name))
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
          kprintf(": %s\n", prop_name);

        if (not match(prop_name))
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
  cur_idx = 0;
  nxt_idx = 0;
  view = {nullptr, 0};
  last = path[1] == '\0';
  found = false;
  if (path[0] == '/')
    traverse(find_path);
  return {found, view};
}
