ARMGNU ?= aarch64-linux-gnu

all: task.o linker.ld
	$(ARMGNU)-ld -T linker.ld -static -o task.elf task.o
	$(ARMGNU)-objcopy task.elf -O binary task.img

task.o: task.S
	$(ARMGNU)-gcc -c $< -ffreestanding

.PHONY: clean dump
clean:
	rm task.d task.elf task.img task.o
dump: all
	qemu-system-aarch64 -M raspi3b -kernel task.img -display none -d in_asm

