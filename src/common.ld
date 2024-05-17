# ref: BCM2837-ARM-Peripherals 1.2 Address map
__phy_heap_start = 0x0001000;
__phy_heap_end   = 0x0050000;
__bootloader     = 0x0060000;
__phy_kernel     = 0x0080000;
__phy_stack_beg  = 0x00A0000;
__phy_stack_end  = 0x00B0000;
__phy_upper_PGD  = 0x00B0000;
__phy_upper_PUD  = 0x00B1000;
__phy_upper_end  = 0x00B2000;

__kernel_space  = 0xFFFF000000000000;
__heap_start    = __kernel_space + __phy_heap_start;
__heap_end      = __kernel_space + __phy_heap_end  ;
__kernel        = __kernel_space + __phy_kernel    ;
__stack_beg     = __kernel_space + __phy_stack_beg ;
__stack_end     = __kernel_space + __phy_stack_end ;
__upper_PGD     = __kernel_space + __phy_upper_PGD ;
__upper_PUD     = __kernel_space + __phy_upper_PUD ;
__upper_end     = __kernel_space + __phy_upper_end ;
