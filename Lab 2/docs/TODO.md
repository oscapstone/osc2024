### Basic Exercise 1 - UART Bootloader - 30%

> Implement a UART bootloader that loads kernel images through UART.

<br>

**From the host to rpi3:**

```python
with open('/dev/ttyUSB0', "wb", buffering = 0) as tty:
    tty.write(...)
```


**QEMU**
```
qemu-system-aarch64 -serial null -serial pty
```


**Kernel Loading Setting:**

add `config.txt` file to your SD card

```
kernel_address=0x60000
kernel=bootloader.img
arm_64bit=1
```

<br>

**Demo**

```
cd uploader

python uploader.py                # when usb connected
python uploader.py /dev/ttys00*   # when using qemu
```

```
screen /dev/tty.usbserial-0001 115200   # macos
screen /dev/ttys00* 115200              # qemu
```

**exit screen**
ctrl-a k

<br>

---

<br>

### Basic Exercise 2 - Initial Ramdisk - 30%

> Parse New ASCII Format Cpio archive, and read file’s content given file’s pathname


```shell
# ls
.
file2.txt
file1
# cat 
Filename: file1
this is file1.
#
```

<br>

**New ASCII Format Cpio Archive**

put all files you need inside `rootfs`

```
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
```

<br>

**Loading Cpio Archive**

qemu

```
-initrd <cpio archive>
```

Rpi3

```
initramfs initramfs.cpio 0x20000000
```

<br>

---

<br>

### Basic Exercise 3 - Simple Allocator - 10%


> Implement a alloc function that returns a pointer points to a continuous space for requested size.

<br>

usage example:
```c
void* simple_malloc(size_t size) {
  ...
}

int main() {
  char* string = simple_alloc(8);
}
```


<br>

---

<br>

### Advanced Exercise 1 - Bootloader Self Relocation - 10%

> Add self-relocation to your UART bootloader, so you don’t need `kernel_address=` option in `config.txt`

<br>


---

<br>

### Advanced Exercise 2 - Devicetree - 30%

> implement a parser that can iterate the device tree. 
> Also, provide an API that takes a callback function, so the driver code can access the content of the device node during device tree iteration.

<br>

**Dtb Loading**

QEMU

```
 -dtb bcm2710-rpi-3-b-plus.dtb
```

Rpi3

> Move `bcm2710-rpi-3-b-plus.dtb` into SD card










