CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -I../include -I../peripherals -fno-stack-protector -g -O0
ASFLAGS = 

# Adjust these paths as necessary
SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.S)
OBJS = $(SRC_C:%.c=%.o) $(SRC_S:%.S=%.o)

# Include peripherals objects
PERIPHERALS_OBJS = $(wildcard $(BUILD_DIR)/peripherals/*.o)
LINKER_SCRIPT = linker.ld
KERNEL8_ELF = test_user.elf
KERNEL8_IMG = test_user.img

all: $(OBJS) kernel

%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

kernel: $(OBJS) $(PERIPHERALS_OBJS)
	$(LD) -T $(LINKER_SCRIPT) -o $(KERNEL8_ELF) $^
	$(OBJCOPY) -O binary $(KERNEL8_ELF) $(KERNEL8_IMG)

clean:
	rm test_user.o test_user.elf test_user.img

.PHONY: all kernel
