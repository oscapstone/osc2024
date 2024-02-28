# Lab0  
[Class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab0.html)
---  
## Some background
+ Cross Compiler  
    - Download cross-compiler(ARM) from apt (```sudo apt install gcc-aarch64-linux-gnu```)
+ Linker
    - TA provides a incomplete pieces of code(linker.ld)
    ```
    SECTIONS
    {
        . = 0x80000;
        .text : { *(.text) }
    }
    ```
    - ```ld --verbose``` can be used to check the content of default liker script file of current system.  
+ QEMU
    - An emulator for cross-platform development(Although QEMU provides a machine option for rpi3, it doesn’t behave the same as a real rpi3).  
    - Install it from apt
---  
## From Source Code to Kernel Image
+ From Source Code to Object Files
    - Try to cross compile a simple source-code file(a.S, actually is assembly) TA provided to generate an .o file.
    ```
    .section ".text"
    _start:
        wfe
        b _start
    ```
    - Compile it using ```aarch64-linux-gnu-gcc -c a.S```
    - In my environment, I only have ```aarch64-none-linux-gnu-gcc``` initially, not sure if they're the same(I think yes). There's no data regarding as far as I could found. According to chatGPT:
    > No, 'aarch64-linux-gnu-gcc' and 'aarch64-none-linux-gnu-gcc' are not the same.

    > 1. **aarch64-linux-gnu-gcc**: This compiler is typically used for building programs to run on a Linux-based system targeting the AArch64 (ARMv8-A) architecture. It generates code that runs on Linux systems, and the compiled programs rely on Linux system calls and libraries.

    > 2. **aarch64-none-linux-gnu-gcc**: This compiler is typically used for bare-metal or embedded development targeting the AArch64 architecture. It generates code that doesn't rely on any particular operating system, hence the "none" part. It's used for building programs that run directly on the hardware without an operating system, or with a minimal runtime environment.

    > The difference between the two lies in the target environment and the libraries they link against. The "linux" in the first compiler's name indicates it's meant for Linux-based development, while the "none" in the second compiler's name indicates it's meant for bare-metal or standalone development without an operating system.
    - Some reference of cross-compiler[ref1](https://stackoverflow.com/questions/13797693/what-is-the-difference-between-arm-linux-gcc-and-arm-none-linux-gnueabi) [ref2](https://blog.csdn.net/gxy199902/article/details/127162898)
+ From Object Files to ELF  
    - Use linker to generate ELF file which can be executed by program loaders(provided by OS).
    ```aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o```
+ From ELF to Kernel Image  
    > Rpi3’s bootloader can’t load ELF files. Hence, you need to convert the ELF file to a raw binary image. You can use objcopy to convert ELF files to raw binary.
    ```aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img```
+ Check on QEMU  
    - Use QEMU to see the dump assembly
    ```qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm```
    - [Regarding the '-d'](https://unix.stackexchange.com/questions/645478/how-to-understand-qemu-d-int-flag-output)
---  
## Deploy to REAL Rpi3
+ Flash Bootable Image to SD Card
    > To prepare a bootable image for rpi3, you have to prepare at least the following stuff.
    > An FAT16/32 partition contains
    > 1. Firmware for GPU.
    > 2. Kernel image.(kernel8.img)
    + I use the image TA provided, you can make it yourself by downloading firmware from [rpi repo](https://github.com/raspberrypi/firmware/tree/master/boot)
        - flash TA's image to SD card.```dd if=nycuos.img of=/dev/sdb```
+ Interact with Rpi3
    - Connect to UART to USB
        - connect TX to RX
        - ```screen /dev/ttyUSB0 115200```
        > After your rpi3 powers on, you can type some letters, and your serial console should print what you just typed.
---  
## Debugging
+ Debug on QEMU
    - First use QEMU to open gdb server:
    ```qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s```
    - Then use gdb to connect to it, but the gdb must also be cross-platform(aarch64-linux-gnu in this class)
        1. Download it from [gnu.org](https://ftp.gnu.org/gnu/gdb/)(choose one version you like, I'm not sure if there will be a problem in much older version)
        2. Uncompress it ```tar -xvf gdb-14.1.tar.xz```
        3. Go to the directory and configure it to generate makefile using ```./configure --host=x86_64-linux-gnu --target=aarch64-linux-gnu --prefix=your/destition/dir```
            - if it ask for GMP and MPC, install them from apt(```udo apt-get install libgmp-dev``` and ```sudo apt-get install libmpc-dev```)
        4. Use ```make -j8``` to compile
        5. ```make install```, and the gdb will be under the your/destition/dir/bin/aarch64-linux-gnu-gdb
    - Execute the aarch64-linux-gnu-gdb, then type ```file kernel8.elf``` and ```target remote :1234```
        - (Not Sure)The remote session will stuck, you can try to type ```continue``` then ^C, you should see you're in _start (). This is due to ```wfe``` in a.S will keep waiting for event
    - add ```-g``` option on both compiler and linker option to generate debugging symbol, but I still not see anything.(At least it won't tell me no debug symbol found)
    - Some references: [ref1](https://blog.csdn.net/xiaoqiaoq0/article/details/109272503) [ref2](https://hackmd.io/@sysprog/gdb-example)