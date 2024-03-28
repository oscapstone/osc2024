_heap_stack_size = 1M;
_bootloader_size = 64K;

SECTIONS
{
    . = 0x80000;//不確定要不要改成0x60000
    _start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        _bss_top = .;
        *(.bss) 
    }
    _bss_size = SIZEOF(.bss) >> 3;
    //計算bss區段的大小 並且8
    //清出一塊記憶體空間 用來裝需要的變數
    .heap : {
        . = ALIGN(8);
        _heap_top = .;
    } 
    //定義heap區段 用於動態分配內存空間
    // ALIGN(8)確保起始地址是八的倍數
    //_heap_top 紀錄起始位址
    // #heap是由下往上
    . = . + _heap_stack_size;
    .stack : {
        . = ALIGN(8);
        _stack_top = .;
    }
    //定義stack區段
    //上往下

    _end = .;
    //bootloader程式結束的地址
    //就是下面的0x3
    . = 0x3000000;
    _bootloader_relocated_addr = 0x3000000;
    . = . + _bootloader_size;
    //bootloader 往下移動他自己大小的距離
}

__code_size = (_end - _start);
