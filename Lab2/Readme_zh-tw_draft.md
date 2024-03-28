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
3. jump into 0x80000

```
void (*kernel_entry)(void *) = (void (*)(void *))0x80000;
kernel_entry(dtb);
```
這行程式碼是在C語言中使用的，特別常見於嵌入式系統和操作系統的開發中。這段代碼涉及到函數指針的使用和函數的調用，我會逐步解釋每部分的含義。

函數指針聲明和初始化:

void (*kernel_entry)(void *)：這是一個函數指針的聲明。指針kernel_entry可以指向一個函數，這個函數接收一個void *類型的參數（即一個指向任何類型的指針）並且不返回任何值（void）。
= (void (*)(void *))0x80000;：這部分初始化了kernel_entry函數指針。它將指針指向記憶體地址0x80000處的函數。這裡，0x80000是一個固定的記憶體地址，通常這樣的地址用於特定目的，比如在系統啟動時跳轉到操作系統的入口點。轉型操作(void (*)(void *))是必要的，以確保編譯器理解這個地址被用作指向上述簽名函數的指針。
函數調用:

kernel_entry(dtb);：這行代碼調用了kernel_entry指向的函數，傳遞dtb作為參數。dtb應該是一個void *類型的變量，雖然原代碼中沒有給出dtb的聲明，但它通常用於傳遞系統或設備的配置信息，例如設備樹（Device Tree Blob，一種描述硬體配置的數據結構）。
整體來說，這段代碼設置一個函數指針指向一個特定的記憶體地址（這個地址上預期有一個函數），然後通過這個函數指針調用那個函數，傳遞dtb作為參數。這種技術常見於需要直接與硬體交互的低級系統編程中，特別是在操作系統的啟動階段。

#### Config
Load bootloader with 64bit to fit ARMv8 (64bit ARM) architecture, since the bootloader is compiled with aarch64 compiler
```
kernel=bootloader.img
arm_64bit=1
```

### Relocation
Rpi 3b+ will automatically load kernel into 0x80000, so the bootloader should relocate itself to 0x60000 to preserve the memory for the kernel, and since the linker linked it to 0x60000, we should load the bootloader to 0x60000 for normal performance

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

4:  bl      bootloader_main-0x20000 //the kernel is moved back 0x20000, so the address to jump should be moved back too
    // for failsafe, halt this core too
    b       1b
```



### Initial Ramdisk
Initial Ramdisk，简称为initrd，是操作系统启动过程中的一个关键组件。在内核初始化之后，操作系统通常需要挂载一个根文件系统并运行一个名为init的用户程序。这个程序可以是一个脚本或可执行的二进制文件，用来启动其他服务或加载额外的驱动程序。

但是，在一些情况下，比如还没有实现任何文件系统和存储驱动的代码，系统就无法从SD卡等存储设备加载任何东西。这时，就可以通过initial ramdisk来加载用户程序。Initial ramdisk是一个由引导程序加载或内嵌于内核中的文件，通常是一个可以被解压以构建根文件系统的归档文件。

为了创建这样的归档文件，我们通常会使用Cpio（copy in, copy out）这种非常简单的归档格式来打包目录和文件。每个目录和文件都以一个头部开始，紧接着是它的路径名和内容。在实验中，你会使用New ASCII Format Cpio格式来创建一个cpio归档。你首先需要创建一个rootfs目录，并将所有需要的文件放置其中。然后，使用特定的命令来归档这个目录。

加载initial ramdisk的方法取决于具体的环境。例如，在QEMU中，可以通过添加参数-initrd <cpio归档文件>来加载cpio归档文件，而在Raspberry Pi 3等硬件上，则可能需要将cpio归档文件移动到SD卡上，并在config.txt中指定文件名和加载地址。

总的来说，initial ramdisk为操作系统提供了一种在还没有访问到真实根文件系统之前，通过一个临时的根文件系统来加载和执行必要程序和驱动的机制。这在操作系统的启动和运行初期阶段尤为重要，特别是在开发和测试新内核或系统时。
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
#### 使用newc的原因
数字字段的表示：newc格式使用8字节的十六进制数来表示所有数字字段，包括inode号、文件模式（权限）、UID、GID、链接数、文件大小等。这种表示方式提高了字段的精度和兼容性，尤其是在支持大文件和大inode号方面。

设备号的分离表示：与旧的二进制格式或PWB格式相比，newc格式将设备号分为主设备号（c_devmajor）和次设备号（c_devminor），以及对应的字符设备或块设备的主次设备号。这提供了更细粒度的设备管理，特别是在现代系统中，设备号范围更广。

对齐和填充：newc格式要求路径名和文件数据在4字节边界上对齐，通过在必要时添加NUL字节来实现。这种对齐有助于提高某些系统上的访问效率，并简化了在不同架构之间移植的处理。

简化的CRC支持：虽然newc格式默认不包含CRC校验（其校验字段c_check总是设为0），但其格式与CRC格式（070702魔数）基本相同，仅除了校验字段的处理。这意味着可以轻松地扩展或修改newc格式以支持文件内容的校验，而不必更换整个存档格式。

广泛的支持：由于其兼容性和灵活性，newc格式已被广泛采用为Linux和其他类Unix系统中cpio存档的首选格式。许多工具和库都支持newc格式，使其成为交换和分发文件系统内容的可靠选择。

使用newc格式创建cpio归档文件，是因为newc是一种新的ASCII格式，它是cpio工具支持的多种格式之一。newc格式的主要特点和优势包括：

新的ASCII格式（newc）：newc代表“new ASCII format with CRC”，即带有CRC的新ASCII格式。它提供了对较新特性的支持，比如较大文件的支持和更好的错误检测能力。

支持大文件：与旧的cpio格式相比，newc格式支持创建大文件的归档。这对于现代系统中常见的大文件来说非常有用。

CRC校验：newc格式包括对每个归档条目的CRC校验和，这增加了数据完整性检查的能力，有助于识别数据传输或存储过程中可能出现的错误。

兼容性：尽管newc是一种较新的格式，它被现代Linux内核在引导过程中用作初始RAM磁盘（initramfs）的默认格式。这意味着newc格式的cpio归档与Linux内核高度兼容，适合用作包含启动所需文件的initramfs。

灵活性和广泛的支持：newc格式的支持在各种Linux发行版和工具中普遍存在，提供了创建、提取和管理cpio归档的灵活性。

综上所述，选择newc格式创建cpio归档是因为它提供了更好的特性集，特别是在处理大文件和数据完整性方面，同时确保与Linux内核的兼容性，这对于initramfs这样的应用场景来说是非常重要的。

### Simple Allocator
Pool 會在 bss
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

Name: NULL terminating
```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
```

進到每個node，遇到prop的時候就會有個struct告訴你他的length跟name，string+name可以幫你去看他的property，而len就是接下來有多長的東西都是他的，比如
```
serial@40002000 {
    compatible = "arm,pl011", "arm,primecell";
    reg = <0x40002000 0x1000>;
    interrupts = <26>;
};
```
就會有三個node (compatible, reg, interrupts)，走過structure以後就會是他的內容。
Target:
```
/ {
	chosen {
		linux,initrd-start = <0x82000000>;
		linux,initrd-end = <0x82800000>;
	};
};
```

假設遇到多層的node
```
&spi0 {
	pinctrl-names = "default";
	pinctrl-0 = <&spi0_pins &spi0_cs_pins>;
	cs-gpios = <&gpio 8 1>, <&gpio 7 1>;

	spidev0: spidev@0{
		compatible = "spidev";
		reg = <0>;	/* CE0 */
		#address-cells = <1>;
		#size-cells = <0>;
		spi-max-frequency = <125000000>;
	};

	spidev1: spidev@1{
		compatible = "spidev";
		reg = <1>;	/* CE1 */
		#address-cells = <1>;
		#size-cells = <0>;
		spi-max-frequency = <125000000>;
	};
};
```
假设你遇到的是一个包含两个子节点（spidev0 和 spidev1）的设备树节点（&spi0）。在解析过程中，解析器首先会遇到 &spi0 节点的开始标记（FDT_BEGIN_NODE），接着解析该节点的各个属性（如 pinctrl-names、pinctrl-0、cs-gpios 等），然后进入子节点 spidev0。

解析 &spi0 节点的属性： 对于每个属性（如 pinctrl-names、pinctrl-0、cs-gpios），解析器会读取属性名在字符串表中的偏移量和属性值。对于复杂的属性值（比如引用其他节点的 <&spi0_pins &spi0_cs_pins> 或 <&gpio 8 1>, <&gpio 7 1>），它们通常以特定格式存储，如设备树节点的引用可能通过 phandle（一个指向其他节点的引用）来实现。

进入子节点 spidev0 和 spidev1 的解析： 当解析器遇到子节点 spidev0 的开始标记时，它将重复类似的解析过程：读取节点名称、解析所有属性（compatible、reg、#address-cells、#size-cells、spi-max-frequency），直到遇到子节点的结束标记（FDT_END_NODE）。

处理子节点内的属性： 在 spidev0 和 spidev1 子节点中，compatible 属性标识了节点兼容的设备类型或模型，reg 属性指定了设备的寄存器地址或设备特定的编号（在这里分别是 0 和 1，代表 CE0 和 CE1），而 spi-max-frequency 指定了 SPI 通信的最大频率。

结束子节点和父节点的解析： 解析完 spidev0 后，解析器将继续前进到下一个节点（如果有的话），在这个例子中是 spidev1。解析完所有子节点后，最终会遇到父节点 &spi0 的结束标记。

在整个解析过程中，解析器会使用一种递归或迭代的方法遍历整个设备树结构，从而能够逐个处理每个节点和属性。重要的是，解析器需要能够正确处理各种数据类型（包括整数、字符串、引用等），并且能够根据设备树规范正确解释这些数据。

#### 對齊

如果节点名称结束后已经对齐到4字节边界，那么根据设备树的规范，不会再添加额外的 NULL 字节进行填充。设备树的设计原则之一是确保每个数据块（包括节点名称和属性值）在4字节边界上对齐，以便于不同架构的处理器高效读取数据。

具体来说，如果一个节点名称（包括结尾的 NULL 字节）的总长度是4的倍数，那么紧接着的数据（可能是下一个属性或另一个节点的开始标记）会直接跟在这个名称后面，不需要额外的填充字节。但如果节点名称的长度加上结尾的 NULL 字节后不是4的倍数，那么将添加足够的 NULL 字节，直到总长度达到最近的4的倍数，以确保后续数据的对齐。

举个例子：

如果节点名称为 "example@"（8个字符，加上结尾的NULL字节共9个字节），为了对齐到4字节边界，会在名称后面添加3个NULL字节进行填充。
如果节点名称为 "example@node"（12个字符，加上结尾的NULL字节共13个字节），为了对齐到4字节边界，会在名称后面添加3个NULL字节进行填充。
如果节点名称为 "node"（4个字符，加上结尾的NULL字节共5个字节），为了对齐到4字节边界，会在名称后面添加3个NULL字节进行填充。
总结来说，是否添加NULL字节取决于节点名称（包括结尾的NULL字节）后是否已经对齐到4字节边界。如果已经对齐，则不再添加NULL字节。

#### Note: Endian
Big Endian（大端序）
定义： 在big endian字节序中，最高位字节（最重要的字节，称为big-end）被存储在最低的内存地址，随后是次高位，以此类推，最低位字节（最不重要的字节）被存储在最高的内存地址。
例子： 假设有一个32位的整数0x12345678，在big endian系统中，它将按照12 34 56 78的顺序存储。
Little Endian（小端序）
定义： 在little endian字节序中，最低位字节（最不重要的字节，称为little-end）被存储在最低的内存地址，随后是次低位，以此类推，最高位字节（最重要的字节）被存储在最高的内存地址。
例子： 同样的32位整数0x12345678在little endian系统中，会被存储为78 56 34 12的顺序。