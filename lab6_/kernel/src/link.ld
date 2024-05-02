_heap_stack_size = 8M;
#heap size 0x800000

# 0x080000   _start
#            text, rodata, bss
#            heap+stack // simple allocator
# 0x880000   _end
# 0x3000000  bootloader
# 0x3100000  _end
# 0x8000000  initramfs
# 0x10000000 buddy_system
# 0x20000000 _end

SECTIONS
{
    . = 0xffff000000000000;
    . += 0x80000;
    _kernel_start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _kernel_end = .;

    _heap_start = .;

    . = 0xffff00002c000000;
    _stack_end = .;

    . = 0xffff00003c000000;
    _stack_top = .;
}

__bss_size = (__bss_end - __bss_start) >> 3;
