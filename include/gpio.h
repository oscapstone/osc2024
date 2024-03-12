/*
 * Copyright (C) 2018 bzt (bztsrc@github)
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
#ifndef GPIO_H
#define GPIO_H

#define __IO            volatile

#define MMIO_BASE       0x3F000000
#define GPIO_BASE       (MMIO_BASE + 0x200000)

typedef struct {
    __IO unsigned int GPFSEL[6];
    unsigned int reserved0;
    __IO unsigned int GPSET[2];
    unsigned int reserved1;
    __IO unsigned int GPCLR[2];
    unsigned int reserved2;
    __IO unsigned int GPLEV[2];
    unsigned int reserved3;
    __IO unsigned int GPEDS[2];
    unsigned int reserved4;
    __IO unsigned int GPREN[2];
    unsigned int reserved5;
    __IO unsigned int GPFEN[2];
    unsigned int reserved6;
    __IO unsigned int GPHEN[2];
    unsigned int reserved7;
    __IO unsigned int GPLEN[2];
    unsigned int reserved8;
    __IO unsigned int GPAREN[2];
    unsigned int reserved9;
    __IO unsigned int GPAFEN[2];
    unsigned int reserved10;
    __IO unsigned int GPPUD;
    __IO unsigned int GPPUDCLK[2];
    unsigned int reserved11;
} gpio_t;

#define GPIO                ((gpio_t *) (GPIO_BASE))

#endif
