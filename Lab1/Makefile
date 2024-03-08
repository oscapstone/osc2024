CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -Iinclude -fno-stack-protector -g
K8 = kernel8
LINKER = linker.ld
BUILD_DIR = build

SRC_DIR = .
SOURCES_C = $(wildcard $(SRC_DIR)/*.c)
SOURCES_S = $(wildcard $(SRC_DIR)/*.S)
OBJECTS = $(SOURCES_C:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) $(SOURCES_S:$(SRC_DIR)/%.S=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(K8).img

qe: $(BUILD_DIR)/$(K8).img
	qemu-system-aarch64 -M raspi3 -kernel $< -serial null -serial stdio

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/$(K8).img: $(OBJECTS)
	@mkdir -p $(@D)
	$(LD) -T $(LINKER) -o $(BUILD_DIR)/$(K8).elf $^
	$(OBJCOPY) -O binary $(BUILD_DIR)/$(K8).elf $@

clean:
	rm -rf $(BUILD_DIR)
