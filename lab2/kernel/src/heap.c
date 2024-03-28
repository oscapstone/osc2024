#include "heap.h"

extern char _heap_top;
static char* htop_ptr = &_heap_top; //heap起始位址

void* malloc(unsigned int size) { //size:請求分配的memory大小
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10; //跳過header的大小
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10; //對齊10的倍數
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;//pointer址向下一塊可用的memory，準備給下一次使用
    return r;
}

/*實作一個簡單的記憶體分配器
在kernel初始化的早期階段，還沒有完整的動態記憶體分配器，來分配或釋放記憶體
所以先暫時實作一個簡易版本，不支援釋放記憶體的功能
稱作heap 分配器

透過一個pointer管理一塊memory
每次分配完，pointer向前移動，指向下一塊可用的memory

*/