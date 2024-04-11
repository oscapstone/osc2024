CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
AR = aarch64-linux-gnu-ar
OBJCOPY = aarch64-linux-gnu-objcopy
LINKER_SCRIPT = linker.ld
START = start
BUILD = ../build

CFLAGS := -fno-stack-protector -I ../include/

all: $(BUILD)/kernel8.img

$(BUILD)/$(START).o: $(START).S
	$(CC) $(CFLAGS) -c $(START).S -o $(BUILD)/$(START).o

OBJS := \
	main.o \
	mbox.o \
	uart.o \
	shell.o \
	string.o \
	reset.o \
	cpio.o

OBJS := $(addprefix $(BUILD)/, $(OBJS))

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel8.img: $(BUILD)/$(START).o $(OBJS)
	$(LD) $(BUILD)/$(START).o $(OBJS) -T $(LINKER_SCRIPT) -o $(BUILD)/kernel8.elf
	$(OBJCOPY) -O binary $(BUILD)/kernel8.elf $(BUILD)/kernel8.img

clean:
	rm -f $(BUILD)/kernel8.elf $(BUILD)/kernel8.img $(BUILD)/*.o

run:
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD)/kernel8.img -serial null -serial stdio

run_with_fs:
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD)/kernel8.img -serial null -serial stdio -initrd ../initramfs.cpio
