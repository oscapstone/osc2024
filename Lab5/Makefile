CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
CFLAGS = -Iinclude -Iperipherals -IDSLibrary -fno-stack-protector -g
ASFLAGS = 
BUILD_DIR = build

# Define peripherals source files
PERIPHERALS_SRC_C = $(wildcard peripherals/*.c)
PERIPHERALS_SRC_S = $(wildcard peripherals/*.S)
PERIPHERALS_OBJS = $(PERIPHERALS_SRC_C:%.c=$(BUILD_DIR)/%.o) $(PERIPHERALS_SRC_S:%.S=$(BUILD_DIR)/%.o)

# Define DSLibrary source files
DSLIBRARY_SRC_C = $(wildcard DSLibrary/*.c)
DSLIBRARY_SRC_S = $(wildcard DSLibrary/*.S)
DSLIBRARY_OBJS = $(DSLIBRARY_SRC_C:%.c=$(BUILD_DIR)/%.o) $(DSLIBRARY_SRC_S:%.S=$(BUILD_DIR)/%.o)

# Target for peripherals
$(BUILD_DIR)/peripherals/%.o: peripherals/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/peripherals/%.o: peripherals/%.S
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

# Target for DSLibrary
$(BUILD_DIR)/DSLibrary/%.o: DSLibrary/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/DSLibrary/%.o: DSLibrary/%.S
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

# all: peripherals bootloader kernel
all: peripherals DSLibrary kernel bootloader

peripherals: $(PERIPHERALS_OBJS)

DSLibrary: $(DSLIBRARY_OBJS)

kernel:
	$(MAKE) -C kernel BUILD_DIR=../$(BUILD_DIR)

bootloader:
	$(MAKE) -C bootloader BUILD_DIR=../$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all peripherals kernel bootloader clean
