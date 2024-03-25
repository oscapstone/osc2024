# OSC2024 Lab2
### Bootloader
1. Place ``config.txt`` and ``bootloader.img`` into SD card.
2. Connect to raspi3b+ with command
```
sudo screen /dev/ttyUSB0 115200
```
3. Type in "boot" in serial port to ensure the loader is waiting for file.
4. Open another terminal in the lab2 folder and use ``send_loader.py`` : 
```
python3 send_loader.py -s [serial path] -f [kernel file]
//default -s /dev/ttyUSB0 -f kernel8.img
```

#### send_loader.py
1. Open and get size of kernel.
2. open serial in wb (write binary) mode
3. send kernel size in little endian (small bit first, 4 bytes (int))
4. sleep to wait for the uart buffer
5. send kernel file

#### Bootloader main
1. use uart_getc() to get byte and save in char array. (4byte -> int)
2. load kernel with the size.
3. jump into 0x80000 (x30: linker)

### Relocation
Rpi 3b+ will automatically load kernel into 0x80000, so the bootloader should relocate itself to 0x60000 to preserve the memory for the kernel, and since the linker linked it to 0x60000, we should load the bootloader to 0x60000 for normal performance
把自己移到 0x60000 後開始執行。
```
.section ".text.boot"

.global _start

_start:
	// relocate bootloader
    mov     x28, x0
	ldr x1, =0x80000 //64 bits for address, 0x1 = 1 byte
	ldr x2, =0x60000 //ldr: put address to register
	ldr w3, =__bootloader_size //size, 32 bit is enough

relocate: //since its 64-bit architecture, move 8 bytes every time
	ldr x4,[x1],#8 //store value from x1 to x4, then mov x1 ([] means read value in the address)
	str x4,[x2],#8 //store value from x4 to x2, then mov x2 ([] means write value in the address)
	sub w3,w3,#1 //size is >>3, so 1 means 8 bytes
	cbnz w3,relocate //keep relocate if size is not 0

init_cpu:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 //get cpu num
    and     x1, x1, #3 //see if cpu num is 0
    cbz     x1, 2f //cpu 0 jump to 2f (others wfe)
    // cpu id > 0, stop

1:  wfe
    b       1b
    
2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start //ldr: load Register
    mov     sp, x1 //stack pointer before _start

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
    
3:  cbz     w2, 4f //see if bss is cleared, if cleared go to 4
    str     xzr, [x1], #8 //xzr means zero, clear to zzero from start
    sub     w2, w2, #1 //1 count means 8bit
    cbnz    w2, 3b //if not zero go back

    // jump to C code, should not return

4:  bl      bootloader_main-0x20000
    // for failsafe, halt this core too
    b       1b
```

#### 關於地址如何被設定(為何bss等不用改，但bootloader_main要)
為甚麼bootloader main會有地址差異，但其他bss等等的東西卻是基於0x60000的?

当您的bootloader执行自我重定位时，这个过程主要涉及将整个程序代码（.text段或更多）从一个地方（比如0x80000）复制到另一个地方（比如0x60000）。这种操作通常是因为程序最初被加载到了一个非预期的内存地址，需要移动到一个特定的地址以满足运行时需求或硬件约束。

关于bootloader_main地址差异
编译和链接时的地址安排：在编译和链接过程中，所有符号（包括函数和变量）的地址都是基于链接器脚本中指定的起始地址（在这个例子中是0x60000）来分配的。这意味着bootloader_main和全局变量（比如.data和.bss段中的变量）的地址都是按照这个基址来计算的。
执行重定位：当bootloader在运行时将自己从0x80000复制到0x60000时，这个操作实际上改变了程序代码在物理内存中的位置，但并不自动更新那些编译时计算出的地址。因此，尽管bootloader_main的代码被移动了，但CPU执行到bl bootloader_main-0x20000这条指令时，需要考虑到这个地址偏差，以确保能正确跳转到新位置的bootloader_main。
关于.bss和其他段的处理
.bss和其他数据段：对于.bss（未初始化数据段）和.data（已初始化数据段）这样的数据段，它们的地址在编译和链接时也是基于0x60000这个基址来安排的。当bootloader执行重定位操作时，通常需要确保这些数据段也被适当地处理，以反映它们在内存中的新位置。这可能涉及到更新指向这些段的指针，或者在重定位过程中复制这些数据到新的地址。
地址修正：对于通过绝对地址访问的符号（如通过直接引用bootloader_main的地址），如果存在基地址的偏移（如从0x80000到0x60000的移动），则需要在代码中显式地考虑这种偏移。然而，对于大多数程序数据（如全局变量），一旦它们被加载或复制到正确的位置，程序内的相对引用（如指针）应该仍然有效，因为这些引用是基于编译时确定的地址偏移。
总之，bootloader_main地址差异的处理特别是因为它是通过一条需要计算绝对地址的指令（如bl）直接访问的，而.bss和其他段的数据通常是通过相对地址或指针访问，这些在程序正确重定位后自然会指向正确的位置。正确管理这些地址和重定位的关键在于确保程序的所有部分都被考虑到，并且在必要时进行地址的适当调整。

bootloader_main的地址到底是基於它加載的位置還是linker提供的?

bootloader_main的地址是基于它加载的位置，而不是链接器提供的位置。

在链接器的工作中，它会决定将bootloader_main函数放置在可执行文件的某个位置，这个位置是相对于链接地址（linker-provided address）的。但是，当程序被加载到内存中执行时，操作系统或者加载程序会将可执行文件加载到内存的某个位置，这个位置是程序的实际加载地址（load address）。因此，bootloader_main的地址是基于它加载到内存中的位置来计算的。

在启动代码中，你可能需要手动减去某个偏移量来调用bootloader_main，以确保程序能够正确执行。这个偏移量通常是由程序加载到内存时的地址与链接地址之间的差异来确定的。

### Initial Ramdisk
```
struct cpio_newc_header {
    char c_magic[6];         // Magic number identifying the CPIO format
    char c_ino[8];           // Inode number
    char c_mode[8];          // File mode and type
    char c_uid[8];           // User ID of file owner
    char c_gid[8];           // Group ID of file owner
    char c_nlink[8];         // Number of hard links
    char c_mtime[8];         // Modification time of file
    char c_filesize[8];      // Size of file (in hexadecimal)
    char c_devmajor[8];      // Major device number (for device files)
    char c_devminor[8];      // Minor device number (for device files)
    char c_rdevmajor[8];     // Major device number for the device file node referenced by the symlink
    char c_rdevminor[8];     // Minor device number for the device file node referenced by the symlink
    char c_namesize[8];      // Size of filename (in hexadecimal)
    char c_check[8];         // Checksum
};
```
先把指針移動超過header後，會指到檔名的地方，可以把它印出來以後，再加上name的size，然後對齊四的倍數就會到檔案內容的地方，可以把它印出來以後，再對齊四的倍數就會是下一個檔案的header。
#### 創建initramfs.cpio
```
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio //-H header newc format
cd ..
```

### Simple Allocator
```
static char memory_pool[MEMORY_POOL_SIZE]; //create a memory pool
static char *next_free = memory_pool; //the current address which is not allocated

void *simple_alloc(int size) {
    if (next_free + size - memory_pool > MEMORY_POOL_SIZE) {
        return 0; 
    }
    void *allocated = next_free; 
    next_free += size; 
    return allocated; 
}
```


### DeviceTree
**Reference**
* DTS code: https://github.com/raspberrypi/linux/blob/rpi-5.10.y/arch/arm/boot/dts/bcm2710-rpi-3-b-plus.dts

```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
```