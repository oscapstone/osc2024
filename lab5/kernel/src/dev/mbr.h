#pragma once

#include "base.h"

// wiki 
// https://zh.wikipedia.org/zh-tw/%E4%B8%BB%E5%BC%95%E5%AF%BC%E8%AE%B0%E5%BD%95

typedef struct PACKED {
    U8 head;
    U16 sector_cylinder;
}PARTITION_INFO;

// total 16 byte each
typedef struct PACKED {
    U8 status;                  // 割區狀態：00-->非活動分割區(不可開機)；80-->活動分割區(可開機)；其它數值沒有意義
    PARTITION_INFO start;       // 分割區起始
    U8 fs_flags;                // 檔案系統標誌位   (檔案系統ID) (ex. 0x01 = FAT12, 0x0b = FAT32, 0x0c = FAT32 (LBA))
    PARTITION_INFO end;         // 分割區結束
    U32 start_sector;           // 分割區起始相對磁區號
    U32 n_sector;               // 分割區總的磁區數
}PARTITION_ENTRY;

// total 512 bytes
typedef struct PACKED {
    char bootstrap_code[446];   // the code for booting this hard drive
    PARTITION_ENTRY pe[4];      // partition entry info
    char magic_code[2];         // the magic code 0x55aa
}MBR_INFO;

// also be block size
#define MBR_DEFAULT_SECTOR_SIZE     512