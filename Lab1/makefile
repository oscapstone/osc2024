C_FLAGS = -nostdinc -nostdlib -ffreestanding -mgeneral-regs-only -Iinclude

OBJ_DIR = obj
OBJ_FILES = obj/main.o obj/shell.o obj/uart.o obj/boot.o obj/utils.o obj/mailbox.o

all: kernel8.img

kernel8.img: kernel8.elf
	llvm-objcopy --output-target=aarch64-rpi3-elf -O binary kernel8.elf kernel8.img

kernel8.elf: $(OBJ_FILES)
	ld.lld -nostdlib -m aarch64elf -T linker.ld -o kernel8.elf $^

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	clang $(C_FLAGS) -mcpu=cortex-a53 --target=aarch64-rpi3-elf -c $< -o $@

$(OBJ_DIR)/%.o: src/%.s
	clang -mcpu=cortex-a53 --target=aarch64-rpi3-elf -c $< -o $@

.PHONY: run debug clean
run: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	rm -rf $(OBJ_DIR) *.img *.elf
