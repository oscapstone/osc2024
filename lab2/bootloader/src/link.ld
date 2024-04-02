_heap_stack_size = 1M;
_bootloader_size = 64K;

SECTIONS
{
    /* 不須改成 0x60000 因為advanced bootloader會overlap原本的檔案：使其變成kernel8.img檔*/
    . = 0x80000;

    _start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }

    /*放置初始化的變數*/
    .data : { *(.data) }

    /*為了清出一段記憶體空間能放置未初始化的變數*/
    .bss : {
        _bss_top = .;
        *(.bss) 
    }
    _bss_size = SIZEOF(.bss) >> 3; // >>3 一次除 2*3 = 八位元
    // heap是由低地址往高地址增加（由下往上）
    .heap : {
        . = ALIGN(8); //將 heap 的地址是八位元組
        _heap_top = .; //紀錄heap的起始位址
    } 
    . = . + _heap_stack_size;
    // stack是由高地址往低地址增加（由上往下）
    .stack : {
        . = ALIGN(8);
        _stack_top = .; //紀錄stack的起始位址
    }
    _end = .; // 其值為當前位置，即Bootloader程式的結束地址。

    . = 0x3000000; //這個表示啟動程式(bootloader)被重新定位到的記憶體地址。
    _bootloader_relocated_addr = 0x3000000;
    // 這個地址就是啟動程式執行時所在的記憶體地址。
    . = . + _bootloader_size;
}

__code_size = (_end - _start); // 計算程式碼的總大小，即從開始到結束的距離。
