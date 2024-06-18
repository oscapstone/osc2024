/*
 * Copyright (C) 2018 bzt (bztsrc//github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "mm.h"
.section ".text.boot"

#define BOOTLOADER

.global _start
_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b
2:  // cpu id == 0, continue

#ifndef BOOTLOADER
    /* get the dtb address */
    mov     x18, x0 // store the dtb address to x18, which is used as the first argument of the main function.
#endif

    // set exception vector table
    bl      set_exception_vector_table

    // change exception level to EL1
    bl      from_el2_to_el1

    /* set up TCR */
    ldr     x0, = TCR_CONFIG_DEFAULT
    msr     tcr_el1, x0

    /* Set up MAIR_EL1 */
    ldr     x0, =MAIR_CONFIG
    msr     mair_el1, x0

    /* At this time, the sp is virtual address, set an proper physical address to setup page table. */
    mov     sp, 0x80000
    bl      setup_page_table

    /* Enable MMU */
    mrs     x2, sctlr_el1
    orr     x2, x2, 1
    //bic     x2, x2, #(1 << 19) // clear WXN bit
    msr     sctlr_el1, x2 // enable MMU, cache remains disabled

    /* Then we have to set stack pointer to proper virtual address */
    ldr     x0, =__KERNEL_START_ADDR
    mov     sp, x0

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
4:
    /* jump to C code, should not return. Using br will load the absolute address of main(). Using bl or b just jump by offset. */
    ldr     x0, =main
    br      x0
    // for failsafe, halt this core too
    b       1b