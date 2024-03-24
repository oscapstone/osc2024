BUILD_DIR = ./build
# Lib
LIB_SRC_DIR = .

LIB_SRC = \
	$(LIB_SRC_DIR)/cpio.c \
	$(LIB_SRC_DIR)/mbox.c \
	$(LIB_SRC_DIR)/reset.c \
	$(LIB_SRC_DIR)/string.c \
	$(LIB_SRC_DIR)/dtb.c \
	$(LIB_SRC_DIR)/malloc.c \
	$(LIB_SRC_DIR)/mmio.c \
	$(LIB_SRC_DIR)/shell.c \
	$(LIB_SRC_DIR)/uart.c \

LIB_INCLUDES = \
	-I $(LIB_SRC_DIR)/include \

LIB_BUILD_DIR = $(BUILD_DIR)/lib
LIB_OBJS := $(patsubst %.c,$(LIB_BUILD_DIR)/%.o,$(notdir $(LIB_SRC)))

# ARM toolchain
CROSS = aarch64-linux-gnu
CC = $(CROSS)-gcc
AS = $(CROSS)-as
LD = $(CROSS)-ld
OC = $(CROSS)-objcopy
OD = $(CROSS)-objdump
SP = $(CROSS)-strip

BUILD_DIRECTORIES = \
	$(LIB_BUILD_DIR) \

PROJ = lib

CCFLAGS = -Wall -O0 -fno-stack-protector
LDFLAGS = -T$(LINKER_FILE) -nostdlib -nostartfiles

.PHONY: clean directories out_elf run
all: directories $(LIB_OBJS)
directories: $(BUILD_DIRECTORIES)

# Compile Object Files ---------------------------------------------------------
$(LIB_BUILD_DIR)/%.o : %.c
	@echo '============================================================'
	@echo ' Building target: $@'
	@echo '============================================================'
	@echo "[+] Building: $<"
	$(CC) $(CCFLAGS) $(LIB_INCLUDES) -g -o $@ -c $<
	@echo "[+] Finish Building: $<"
	@echo ''

$(BUILD_DIRECTORIES):
	@echo $(LIB_BUILD_DIR)
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)