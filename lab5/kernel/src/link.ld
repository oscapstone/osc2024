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
    . = 0x80000;
    _start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        _bss_top = .;
        *(.bss) 
    }
    _bss_size = SIZEOF(.bss) >> 3;
    .heap : {
        . = ALIGN(8);
        _heap_top = .;
    } 
    . = . + _heap_stack_size;
    .stack : {
        . = ALIGN(8);
        _stack_top = .;
    }
    _end = .;
}
