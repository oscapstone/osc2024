#ifndef _DTB_H_
#define _DTB_H_

#define uint32_t unsigned int

// manipulate device tree with dtb file format
// linux kernel fdt.h
// 以下為各種類型的tokens(標記)
#define FDT_BEGIN_NODE 0x00000001   //表示一個設備樹節點的開始。
#define FDT_END_NODE 0x00000002     //表示一個設備樹節點的結束。
#define FDT_PROP 0x00000003         //表示一個設備樹節點的屬性。
#define FDT_NOP 0x00000004          //表示一個空操作，通常用於填充或對齊。
#define FDT_END 0x00000009          //表示設備樹的結束。

typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

uint32_t uint32_endian_big2lttle(uint32_t data);
void traverse_device_tree(void *base, dtb_callback callback);  //traverse dtb tree
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size);

#endif
