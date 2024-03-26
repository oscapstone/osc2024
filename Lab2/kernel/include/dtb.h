#ifndef DTB_H
#define DTB_H
#include "def.h"
#include "int.h"

#define FDT_HEADER_MAGIC 0xd00dfeed


/* Memory Reservation Block
 * list of pairs of 64-bit big-endian integers
 */
struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

#define FDT_TRAVERSE_SUCCESS   0
#define FDT_TRAVERSE_ERROR     1
#define FDT_HEADER_MAGIC_ERROR 2

typedef void (*fdt_callback)(uint32_t token,
                             const char* name,
                             const void* data,
                             uint32_t size);

uintptr_t get_dtb_ptr(void);
void set_dtb_ptr(uintptr_t);

uint32_t fdt_traverse(fdt_callback cb);

void fdt_find_cpio_ptr(uint32_t token,
                       const char* name,
                       const void* data,
                       uint32_t UNUSED(size));

void print_dtb(uint32_t token,
               const char* name,
               const void* UNUSED(data),
               uint32_t UNUSED(size));

#endif /* DTB_H */
