OBJCOPY := aarch64-linux-gnu-objcopy
CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld

CFLAGS = -Wall -nostdlib

TARGET = kernel8.img
TARGET_ELF = kernel8.elf

all := $(TARGET)

.PHONY : clean

$(TARGET): $(TARGET_ELF)
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

$(TARGET_ELF): linker.ld a.o
	$(LD) -T linker.ld -o kernel8.elf a.o

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	rm $(TARGET) $(TARGET_ELF) *.o