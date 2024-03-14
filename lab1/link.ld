/* 定義內存分佈，包括text, data, bss section位置 */
/* .text, .rodata, .data, .bss 分别对应程序的代码段、只读数据段、初始化数据段和未初始化数据段（BSS）。*/
/* 当链接器链接多个目标文件时，它会根据链接脚本中的规则将所有 .text 段的内容合并到一个 .text 段中，所有 .data 段的内容合并到一个 .data 段中，以此类推。*/
SECTIONS
{
    . = 0x80000; /*程序起始位置*/
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .); /*设置_data符号为当前位置*/
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : { /*放沒有初始化的*/
        . = ALIGN(16); /*将当前位置对齐到16字节，确保后续的数据按照一定的边界对齐存放。*/
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3; /*>>3是bit變byte*/