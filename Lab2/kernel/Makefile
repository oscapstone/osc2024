CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -I../include -I../peripherals -fno-stack-protector -g
ASFLAGS = 

# Adjust these paths as necessary
SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)
OBJS = $(SRC_C:%.c=$(BUILD_DIR)/kernel/%.o) $(SRC_S:%.S=$(BUILD_DIR)/kernel/%.o)

# Include peripherals objects
PERIPHERALS_OBJS = $(wildcard $(BUILD_DIR)/peripherals/*.o)
LINKER_SCRIPT = linker.ld
KERNEL8_ELF = $(BUILD_DIR)/kernel/kernel8.elf
KERNEL8_IMG = $(BUILD_DIR)/kernel/kernel8.img

all: $(OBJS) kernel

$(BUILD_DIR)/kernel/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

kernel: $(OBJS) $(PERIPHERALS_OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $(KERNEL8_ELF) $^
	$(OBJCOPY) -O binary $(KERNEL8_ELF) $(KERNEL8_IMG)

.PHONY: all kernel
