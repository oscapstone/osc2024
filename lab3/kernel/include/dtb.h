#ifndef _DTB_H_
#define _DTB_H_

#define uint32_t unsigned int

// Structure block node token
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

void parse_dtb_tree(void *dtb_ptr, dtb_callback callback);
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size);

#endif