#ifndef _DTB_H_
#define _DTB_H_

#define uint32_t unsigned int

// manipulate device tree with dtb file format
// linux kernel fdt.h
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

uint32_t uint32_endian_big2little(uint32_t data);
void traverse_device_tree(void *base, dtb_callback callback); // traverse dtb tree
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size);

#endif