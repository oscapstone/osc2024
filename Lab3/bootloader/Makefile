CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -I../include -I../peripherals -fno-stack-protector -g
ASFLAGS = 

# Adjust these paths as necessary
SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)
OBJS = $(SRC_C:%.c=$(BUILD_DIR)/bootloader/%.o) $(SRC_S:%.S=$(BUILD_DIR)/bootloader/%.o)

# Include peripherals objects
PERIPHERALS_OBJS = $(wildcard $(BUILD_DIR)/peripherals/*.o)
LINKER_SCRIPT = linker.ld
BOOTLOADER_ELF = $(BUILD_DIR)/bootloader/bootloader.elf
BOOTLOADER_IMG = $(BUILD_DIR)/bootloader/bootloader.img

all: $(OBJS) bootloader

$(BUILD_DIR)/bootloader/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bootloader/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

bootloader: $(OBJS) $(PERIPHERALS_OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $(BOOTLOADER_ELF) $^
	$(OBJCOPY) -O binary $(BOOTLOADER_ELF) $(BOOTLOADER_IMG)

.PHONY: all bootloader
