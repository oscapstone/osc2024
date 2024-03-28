#ifndef _DTB_H_
#define _DTB_H_

#define uint32_t unsigned int

// manipulate device tree with dtb file format
// linux kernel fdt.h
//每一個struct block由一系列片段組成，每個片段由各個token開始
#define FDT_BEGIN_NODE 0x00000001 //標記一個節點的開始
#define FDT_END_NODE 0x00000002 //標記一個節點的結束
#define FDT_PROP 0x00000003 //標記一個屬性的開始，後面跟隨屬性詳細的資料
#define FDT_NOP 0x00000004 //沒有額外數據，不作為
#define FDT_END 0x00000009 //標記struct block的結束

//dtb_callback : 函數指針類型 指向回調函數
//回調函數 : 對DT的節點，進行操作或提取信息
//在解析DT的過程呼叫callback，可以針對節點，也就是不同的device操作
//ex: 初始化裝置，收集裝置訊息
typedef void (*dtb_callback)(uint32_t node_type, char *name, void *value, uint32_t name_size);

uint32_t uint32_endian_big2lttle(uint32_t data);
void traverse_device_tree(void *base, dtb_callback callback);  //traverse dtb tree
void dtb_callback_show_tree(uint32_t node_type, char *name, void *value, uint32_t name_size);
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size);

#endif
