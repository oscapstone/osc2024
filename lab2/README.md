先把bootloader燒在0x60000，然後透過sendFileByUART.py把kernel.img傳到0x80000，在用bootloader boot他。 (bootloader我們自己寫)

# UART bootloader

## Compile and Flash bootloader to SD card
```
# export llvm path
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

# compile
make 

# 燒bootloader的img到SD卡 (kernel8.img)
rm /Volumes/NO\ NAME/kernel8.img
cp kernel8.img /Volumes/NO\ NAME
```

## run on rasp3
```
# 在mac 用screen 會有resource busy的問題，所以改用minicom (Linux不會)
minicom -D dev/tty.usbserial-0001

# 在bootloader shell裡 input "booting"時在本地跑
python uploader.py
```

# Initial Ramdisk 
在/shell裡面，要先把config.txt 和 initramfs.cpio寫到SD卡裡面，這樣再通電的時候他才會把initramfs.cpio放到rasp3 memory裡的正確位置。

```
# write file system into cpio format
cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
# copy to SD card
cp config.txt /SD_mount_path
cp initramfs.cpio /Volume/SD_mount_path
```

# device tree
先把bcm2710-rpi-3-b-plus.dtb，丟到SD卡

