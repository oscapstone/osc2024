.PHONY: all clean run

CC = aarch64-linux-gnu
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles
ASMFLAGS = -Wall
LINKFLAGS = # -nostdlib -nostartfiles
LINKER = linker.ld

BUILD_DIR = build/

C_SOURCE = $(wildcard *.c)
ASM_SOURCE = $(wildcard *.S)

# build the files in the build directory to look cleaner
C_OBJ = $(addprefix $(BUILD_DIR), $(C_SOURCE:.c=_c.o))
ASM_OBJ = $(addprefix $(BUILD_DIR), $(ASM_SOURCE:.S=_s.o))
OBJ = $(C_OBJ) $(ASM_OBJ)


TARGET = kernel8
ELF = $(BUILD_DIR)$(TARGET).elf
IMG = $(TARGET).img

all: $(BUILD_DIR) $(IMG)

$(BUILD_DIR) :
	mkdir $(BUILD_DIR)

$(C_OBJ) : $(BUILD_DIR)%_c.o : %.c
	$(CC)-gcc $(CFLAGS) -o $@ -c $<

$(ASM_OBJ) : $(BUILD_DIR)%_s.o : %.S
	$(CC)-gcc $(ASMFLAGS) -o $@ -c $<

$(ELF): $(OBJ) $(LINKER)
	$(CC)-ld $(LINKFLAGS) -T $(LINKER) -o $@ $(OBJ)

$(IMG): $(ELF)
	$(CC)-objcopy -O binary $^ $@

run: $(IMG)
	qemu-system-aarch64 -M raspi3b -kernel $^ -display none -d in_asm

debug_run: $(IMG)
	qemu-system-aarch64 -M raspi3b -kernel $^ -display none -S -s

uart_run: $(IMG)
	qemu-system-aarch64 -M raspi3b -kernel $^ -display none -serial null -serial stdio

clean:
	rm -r $(BUILD_DIR)
	rm $(IMG)
