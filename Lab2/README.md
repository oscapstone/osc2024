# Lab2
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab2.html)
---
## Basic Exercises
### Basic Exercise 1 - UART Bootloader
+ background
> In Lab 1, you might experience the process of moving the SD card between your host and rpi3 very often during debugging. You can eliminate this by introducing another bootloader to load the kernel under debugging.
> There are 4 steps before your kernel starts its execution.
> 1. GPU executes the first stage bootloader from ROM on the SoC.
> 2. The first stage bootloader recognizes the FAT16/32 file system and loads the second stage bootloader bootcode.bin from SD card to L2 cache.
> 3. bootcode.bin initializes SDRAM and loads start.elf
> 4. start.elf reads the configuration to load a kernel and other data to memory then wakes up CPUs to start execution.
> The kernel loaded at step 4 can also be another bootloader with more powerful functionalities such as network booting, or ELF loading.

+ Build a new bootloader kernel
    - boot.S
        - Since GPU always start at 0x80000, in order to run our bootloader, we first need to move out code(bootloader) from 0x80000 to address where linker declared(0x60000)
        - The ```_start``` in assembly stands for the begining of assembly, yet when we load new code into ```_start``` it's actually loaded into address pointed by ```_start```. Which are not equlivent, so there should not be overlapping problem
        - According to [ref](https://github.com/bztsrc/raspi3-tutorial/blob/master/14_raspbootin64/start.S), this relocate will only run on BSP core, so there no need to stop other cores. And below is the explaintion of GPT:
        > 1. Execution on BSP Core: The code is designed to run only on the Boot Strap Processor (BSP) core. The BSP core is typically responsible for bootstrapping the system and initializing other cores in a multi-core system. Since this code is intended to run early in the boot process, it assumes that it will execute on the BSP core.
        > 2. No Need to Check CPU's ID: In a multi-core system, each CPU core typically has a unique identifier. Normally, code that needs to run only on a specific core would check the CPU's ID to determine if it's running on the correct core. However, due to the specific firmware change mentioned, there's no need to perform this check in this case. The assumption is that the code will always execute on the BSP core.
        > 3. Spin-Loop on Non-Relocated Address: The comment warns about the consequences of running a spin-loop on a non-relocated address. A spin-loop is a tight loop that continuously checks a condition until it becomes true. If such a loop were to execute on a non-relocated address, it could cause issues, possibly due to the lack of proper initialization or the presence of unexpected data at that address.
        - The rest are the same as Lab1

        - Regarding saving registers before loading new kernel:
    - linker.ld
        - Since 0x80000 is for new kernel, our .text start at 0x60000
        - Record the bootloader size so we can load them to 0x60000 in ```boot.S```
    - bootloader_main.c
        - ```char *kernel_addr = (char *)0x80000``` used to store where to put our loaded kernel.
        > a char pointer has the same alignment requirement as a void pointer.
        - Use a '!' as marker to indicate the transmission is about to start
            - Since on RPI, there's a high chance that a character somehow magically showed up before size is transmitted
        - ```kernel_size``` used to receive kernel size(in bytes) in little endian form.
        - Incrementally put received kernel data into 0x80000
        - Jump to 0x80000 to load new kernel
    - upload.py
        - Calculate kernel size in bytes and send them through serial in little endian form.
        - Send kernel through serial in bytes
+ Testing
    1. ```qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial pty```
    2. run ```upload.py```
    3. ```screen /dev/pts/7 115200``` to check the output(may not be '7', qemu will tell you which number it is)
+ Testing(Rpi)
    1. Wait until blue light on UART flash
    2. ```screen /dev/ttyUSB0 115200```
    3. ```ctrl+a``` then ```:quit```
    4. run ```upload.py```
    5. ```screen /dev/ttyUSB0 115200```
### Basic Exercise 2 - Initial Ramdisk 
+ Background
> After a kernel is initialized, it mounts a root filesystem and runs an init user program. The init program can be a script or executable binary to bring up other services or load other drivers later on.
> However, you haven’t implemented any filesystem and storage driver code yet, so you can’t load anything from the SD card using your kernel. Another approach is loading user programs through the initial ramdisk.
> An initial ramdisk is a file loaded by a bootloader or embedded in a kernel. It’s usually an archive that can be extracted to build a root filesystem.

+ CPIO
    - First we define cpio new ASCII header struct and cpio address in ```cpio.h```
    > QEMU loads the cpio archive file to 0x8000000 by default.
    - Then implement ```ls``` and ```cat``` function
        - ```ls``` requires us to scan whole cpio file and get corresponding file name. A worth mentioning point is that we have to correctly calculate the address so can we get next file header.
        > Each file has a 110 byte header, a variable length, NUL terminated file name, and variable length file data. A header for a file name "TRAILER!!!" indicates the end of the archive.
        > The  pathname  is  followed  by NUL bytes so that the total size	of the fixed header plus pathname is a multiple	of four.  Likewise,  the  file data is padded to a multiple of four bytes.
        ---
        - I implemented ```cat``` through linear search, which I'm certain that there's a better(like building a array to store all files at the begining)
        - We can tell if it's a directory or file through filesize. 
+ Testing
    - run ```qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -serial null -serial stdio``` as we additionally specify ```-initrd``` flag 
### Basic Exercise 3 - Simple Allocator
+ Background
> Kernel needs an allocator in the progress of subsystem initialization. However, the dynamic allocator is also a subsystem that need to be initialized. So we need a simple allocator in the early stage of booting.

+ Get space from heap
    - As we define ```__end``` at the end of our linker script, which hold the address above all .bss, .text, .data. It's actually the address where heap begins(at least it's the case where x86 memory architecture looks like)
    - Yet in order to access the value of ```__end```, we can't just directly access it by calling ```__end```, as [ref](https://stackoverflow.com/questions/8398755/access-symbols-defined-in-the-linker-script-by-application) said:
    > Linker scripts symbol declarations, by contrast, create an entry in the symbol table but do not assign any memory to them. Thus they are an address without a value.
    > This means that you cannot access the value of a linker script defined symbol - it has no value - all you can do is use the address of a linker script defined symbol.
    - So we use ```&__end``` to get address of heap .
    - We record the offset of heap to get new unused heap memory.
---
## Advanced Exercises
### Advanced Exercise 1 - Bootloader Self Relocation
+ Background
> In the basic part, you are allowed to specify the loading address of your bootloader in config.txt. However, not all previous stage bootloaders can specify the loading address. Hence, a bootloader should be able to relocate itself to another address, so it can load a kernel to an address overlapping with its loading address.
+ As it turned out I already finished it in basic exercise 1 part, which are the modifications of ```boot.S``` and ```linker.ld```
### Advanced Exercise 2 - Devicetree
+ Background
+ dtb format
    - Header
    - Memory Reservation Block
    > Each pair gives the physical address and size in bytes of a reserved memory region. These given regions shall not overlap
    > each other. The list of reserved blocks shall be terminated with an entry where both address and size are equal to 0. Note
    > that the address and size values are always 64-bit. On 32-bit CPUs the upper 32-bits of the value are ignored
    > Each uint64_t in the memory reservation block, and thus the memory reservation block as a whole, shall be located at an 8-byte aligned offset from the beginning of the devicetree blob
+ Get dtb address from .S
    - Since dtb address will be loaded into ```x0``` reg, we need a global address in our assembly so that our c files can access that symbol containing address.
    - declare a variable with 64bits long and make it global, remember to put that variable at ```.data``` section. Use ```.quad``` to make it 64bits.
+ utils.c
    - The magic field in dtb header is big endian, so we have to find a way to make it little endian.
    - Add a new function ```unsigned int BE2LE(unsigned int BE)``` to convert to little endian.
