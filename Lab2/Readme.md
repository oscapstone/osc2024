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
/*
kernel_entry: a function pointer point to 0x80000, which recieves a void * as input
*/
void (*kernel_entry)(void *) = (void (*)(void *))0x80000; //jump into loaded kernel
kernel_entry(dtb); // (dtb is saved in x0 for kernel loaded)
```

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
    //mov     x28, x0
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
A temporary root filesystem loaded after initialzing kernel. (SD card is not loaded, can still load file in archive file) (for init)
```
struct cpio_newc_header {
    char c_magic[6];         // Magic number identifying the CPIO format 070701
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

First point to the fs pointer, get the name_size and file_size from header, then skip the header address (110). After that, you can check the filename (Terminal if see TRAILER!!!).

Since newc is aligned with 4 byte, move pointer with name size and align. Then the pointer is at the address of file content. After that, move the pointer with filesize and align to the next file header. 

#### initramfs.cpio
（copy in, copy out）header -> filename -> file content
```
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio //-H header newc format
cd ..
```
#### why using newc?
* newc use 8-byte hex for saving the data and 4-byte align , which is compatible for many device.
* newc is widely used as default(standard) ramdisk format in many Linux or UNIX cpio archive format 
* Easier structure


### Simple Allocator
The pool will be located in bss since it is uninitialized data
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
* Specification: https://www.devicetree.org/specifications/
* chosen: https://www.kernel.org/doc/Documentation/devicetree/bindings/chosen.txt

```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
```
Name: NULL terminating
(Name -> NULL -> prop_token -> struct -> prop -> END)

#### fdt_tranverse
Tranverse into every node, get struct which contains len of prop and name address when meet prop. String + name -> property name, len -> property size.
```
while (*address == FDT_PROP_TOKEN)
        {
            /*
            struct {
                uint32_t len;
                uint32_t nameoff;
            }
            */
            address++;

            // get the length of attribute
            int len = big_to_little_endian_add(address);
            address += 4;

            // get the length to find the target attribute
            int temp = big_to_little_endian_add(address);
            address += 4;

            // if the attribute is correct, get the attribute address
            if (strcmp(string_address + temp, target) == 0)
            {
                /* The /chosen node does not represent a real device in the system but describes parameters chosen or specified by 
                the system firmware at run time. It shall be a child of the root node. */
                callback((char *)big_to_little_endian_add(address));
                uart_puts("found initrd!");
                uart_puts("\n");
                uart_send('\r');
            }

            // jump the value of the attribute
            address += len;
            while (*(address) == NULL){
                address++;
            }
        }
```

Example:
```
serial@40002000 {
    compatible = "arm,pl011", "arm,primecell";
    reg = <0x40002000 0x1000>;
    interrupts = <26>;
};
```
Contains three Nodes

#### Target: linux,initrd-start
The /chosen node does not represent a real device in the system but describes parameters chosen or specified by the system firmware at run time. It shall be a child of the root node.
```
/ {
	chosen {
		linux,initrd-start = <0x82000000>;
		linux,initrd-end = <0x82800000>;
	};
};
```

#### Multilayer Node
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
PROP -> PROP -> PROP -> NODE_BEGIN -> PROP -> ... -> NODE_END


#### Alignment
{name} + NULL (if %4 != 0 -> add NULLs) 


#### Note: Endian
Big Endian
largest digit will be keep in lowest address (easy for human to watch)
0x12345678，12 34 56 78

Little Endian
lowest digit will be keep in lowest address
0x12345678，78 56 34 12

#### Note: callback function
can be called when a specific event is triggered