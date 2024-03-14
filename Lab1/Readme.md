# OSC2024 Lab 1
* Nice Reference: https://github.com/bztsrc/raspi3-tutorial/tree/master
### Assembly List
```
mrs //move to register from state register (for processer)
cbz //compare and branch if zero
ldr //load register
str //store
cbnz //compare and branch if not zero
bl //branch with link
```
### Basic Initialization
files: ``linker.ld``, ``start.S``.
#### start.S
* Initialize CPU settings 
* Clear bss segment 
* Set stack pointer before _start, and jump to main loop.

#### Linker
Links the object files into one executable(``kernel8.img`` in this case), relocates text, data in several object files.
* Load kernel to 0x80000
* Present data at correct memort address and set program counter
```
.text //commands
.rodata //read only data
.data //read-write data, for example: global variables
.bss //not initialized varibles
```

```
SECTIONS
{
    . = 0x80000; /*start loading from 0x80000*/
    
    /*: {} means to combine sub-part into the section. 
    The sub-parts might be generated to .o files by compiler*/
    /* KEEP ensures the symbol will not be discarded after optimizing */
    
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .); /* let _data = current address */
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : { /*don't require space in executable*/
        . = ALIGN(16); /*align with 16 bit for efficiency*/
        /*put all .bss, .bss. segments and other segments into .bss */
        __bss_start = .; 
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;
    
    /* some unneed segments */
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3; /* bss size for start.S */
```

While booting, the device runs _start, which initializes the device and then jumps to main loop.
### Mini UART (Universal Asynchronous Receiver/Transmitter)
#### MMIO (Memory-Mapped IO)
Map hardware on device into a memory address for IO.
#### GPIO
PIN on the board which can be configured with MMIO.
``gpio.h`` sets the memory of the pins and can be visited to get or set hardware info.
#### Initialize UART
Set UART parameters:
```
*AUX_ENABLE |=1;       // enable UART1, AUX mini uart
*AUX_MU_CNTL = 0;      // first disable transmit and recieve
*AUX_MU_LCR = 3;       // 8 bits
*AUX_MU_MCR = 0;       // don't need auto flow ctrl
*AUX_MU_IER = 0;       // disable interrupts
*AUX_MU_IIR = 0xc6;    // disable interrupts
*AUX_MU_BAUD = 270;    // 115200 baud (250M/8x270 = 115200)
```
Map UART to GPIO pins
```
r=*GPFSEL1;
r&=~((7<<12)|(7<<15)); // clear gpio14, gpio15
r|=(2<<12)|(2<<15);    // alt5 (set gpio14, gpio15)
*GPFSEL1 = r;
*GPPUD = 0;            // disable GPIO pull up/down
r=150; while(r--) { asm volatile("nop"); } //wait for enable
*GPPUDCLK0 = (1<<14)|(1<<15); // apply to GPIO 14, 15
r=150; while(r--) { asm volatile("nop"); }
*GPPUDCLK0 = 0;        // flush GPIO setup
*AUX_MU_CNTL = 3;      // enable Tx, Rx
```
Busy-Wait to see if transmitter is ready for send
```
void uart_send(unsigned int c) {
    /* wait until we can send */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    *AUX_MU_IO=c;
}
```
Busy-Wait to see if receiver gets a character
```
char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}
```
Get hex value of address (for mailbox)
```
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad (for bits)
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}
```
Iterating char* with uart_send to send a string.
### Simple Shell
Save the received character into buffer and compare with the command when recieved '\n'.
### Mailbox
Define buffer for mailbox message
```
volatile unsigned int  __attribute__((aligned(16))) mailbox[36];
```
macro (partial)
* MBOX_READ: receive message from GPU
* MBOX_WRITE: write message to GPU
* MBOX_RESPONSE: to see if the response is correct
* MBOX_FULL: check status (don't write if MBOX is full)
* MBOX_EMPTY: check status (don't read if MBOX is empty)
* MBOX_CH_PROP: channel index, 8 is for connection with GPU
mbox_call
```
int mbox_call()
{
    unsigned char ch = MBOX_CH_PROP;
    //combine address of mailbox and the channel index
    unsigned int r = (((unsigned int)((unsigned long)&mailbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mailbox[1]==MBOX_RESPONSE;
    }
    return 0;
}
```
get_{info}: set the content of mailbox and call mbox_call, for example
```
int get_board_revision(){
    mailbox[0] = 7 * 4; // buffer size in bytes, which contains space of require, reply and content 
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = END_TAG;

    return mbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
}
```
**note:** The setting value can be found in https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface

#### Output
```
Board Revision:                 0x00A02082 // Board Device Code
ARM Memory Base Address:        0x00000000 // GPU start address
ARM Memory size:                0x3C000000 // GPU total memory
```

### Reboot
* set: write value into address to execute operation/
* PM_RSTC: write command of booting/rebooting (reset)
* PM_WDOG: count down for command

### Makefile
This Makefile is for Windows.
* Wildcard: Match all file with name *.c
* $< : dependencies
* $@ : target files
```
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: clean kernel8.img

start.o: start.S
	aarch64-none-elf-gcc -c start.S -o start.o

%.o: %.c
	aarch64-none-elf-gcc -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-none-elf-ld start.o $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-none-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	del kernel8.img kernel8.elf *.o 2>nul || echo off

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -d in_asm
```