# OSC2024 Lab0
### 1. Cross Compiler
#### Windows
gcc-arm-10.3-2021.07-mingw-w64-i686-aarch64-none-elf.tar.xz

[Link](https://developer.arm.com/downloads/-/gnu-a)

#### Linux
```
sudo apt-get install make binutils gcc-aarch64-linux-gnu
```

#### Cross Compile
Assembly to object file (Windows will be skipped later)
```
aarch64-linux-gnu-gcc -c a.S (linux)
aarch64-none-elf-gcc -c a.S (Windows)
```
Object file to ELF
```
aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o
```
ELF to Kernel Image
```
aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
```

### 2. QEMU
The system **qemu-system-aarch64** Should be installed.

#### Windows
[Link for Windows](https://qemu.weilnetz.de/w64/)
File: qemu-w64-setup-20231224.exe

After installation, Add ``C:\Program Files\qemu`` into PATH.

#### Linux
Use following command to install
```
sudo apt install qemu-system
```


Use following command to check if the installation is success
```
qemu-system-aarch64 --version
```
Use following commmand to check is raspi3b in the list
```
qemu-system-aarch64 -machine help
```

#### QEMU for Lab 0/1
Because we use UART1 and the argument -serial redirects UART0, **an additional argument -serial is required to monitor UART1.**
```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none
#first -serial null for UART0, second -serial stdio for UART1.
#display none to disable GUI
```

### Bootable Image
* The command dd directly cover original SD card, files can be found in the partition after executing. 
* Replace kernel8.img with your kernel for upcoming Labs.
```
dd if=nycuos.img of=/dev/sdb
# of = the drive of the SD card (Whole drive, not partition)
```
###  Deploy on Rpi3
Connect UART cable and plug the USB to the host machine.
Use screen to connect to the serial port.
```
screen /dev/ttyUSB0 115200
```

