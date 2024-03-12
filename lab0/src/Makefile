CC := aarch64-elf-gcc
LD := aarch64-elf-ld
OBJCOPY := aarch64-elf-objcopy

CFLAGS = -Wall -nostdlib

TARGET = kernel8.img
TARGET_ELF = kernel8.elf

all: $(TARGET)

.PHONY: clean

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_ELF): a.o
	$(LD) -T linker.ld $< -o $@

$(TARGET): $(TARGET_ELF)
	@mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	$(RM) $(TARGET) $(TARGET_ELF) *.o