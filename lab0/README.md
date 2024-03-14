# OSDI2024 macos setup
source code 是 assembly

```
clang -mcpu=cortex-a53 --target=aarch64-rpi3-elf -c lab0.S

llvm-objcopy --output-target=aarch64-rpi3-elf -O binary kernel8.elf kernle8.img
```

安裝llvm
```
https://www.jianshu.com/p/e4cbcd764783

# 要用的話
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

```

燒img到SD卡
```
diskutil list # list all disk (choose external one)
diskutil unmountdisk /dev/disk5

sudo dd if=nycuos.img of=/dev/disk5
```

screen
```
ls /dev/tty* | grep usb

```

