CC=aarch64-linux-gnu-gcc-10
CFLAGS=-Wall $(INCLUDE_DIR) -ffreestanding -g
LD=aarch64-linux-gnu-ld
LD_SCRIPT=linker.ld
OBJCOPY=aarch64-linux-gnu-objcopy

INCLUDE_DIR=-I $(SRC_DIR) -I $(KERNEL_DIR)
SRC_DIR=../src
KERNEL_DIR=.
DIST_DIR=dist

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
SRC_OBJ_FILES=$(SRC_FILES:.c=.o)

KERNEL_FILES=$(wildcard $(KERNEL_DIR)/*.c)
KERNEL_OBJ_FILES=$(KERNEL_FILES:.c=.o)

.PHONY: clean run run-mini-uart

all: kernel8.img

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary \
		$(DIST_DIR)/kernel8.elf	\
		$(DIST_DIR)/kernel8.img

kernel8.elf: start.o $(SRC_OBJ_FILES) $(KERNEL_OBJ_FILES)
	mkdir -p $(DIST_DIR)
	$(LD) -T $(LD_SCRIPT) -o $(DIST_DIR)/kernel8.elf \
		start.o $(SRC_OBJ_FILES) $(KERNEL_OBJ_FILES)

# THIS FUCKED UP DEPS
# start.o: $(SRC_DIR)/start.S
# 	$(CC) $(CFLAGS) -o $(SRC_DIR)/$@ -c $<

# $(SRC_OBJ_FILES): $(SRC_FILES)
# 	$(CC) $(CFLAGS) -o $@ -c $<

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -o $@ -c $<

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -o $@ -c $<

start.o: start.S
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -o $@ -c $<

clean:
	rm start.o $(SRC_OBJ_FILES) $(KERNEL_OBJ_FILES)
	rm -rf $(DIST_DIR)

run:
	qemu-system-aarch64 -M raspi3b -kernel $(DIST_DIR)/kernel8.img -display none -d in_asm

run-mini-uart:
	qemu-system-aarch64 -M raspi3b -kernel $(DIST_DIR)/kernel8.img -display none -serial null -serial stdio
