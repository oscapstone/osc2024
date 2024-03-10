CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy

CFLAGS = -Wall -nostdlib

TARGET = kernel8.img
TARGET_ELF = kernel8.elf

all: $(TARGET)

.PHONY: clean

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_ELF): a.o
	$(LD) -T lab0.ld $< -o $@

$(TARGET): $(TARGET_ELF)
	@mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@

print_linker:
	$(LD) -M -T lab0.ld a.o -o $(TARGET_ELF)

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	$(RM) $(TARGET) $(TARGET_ELF) *.o

