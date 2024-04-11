CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
AR = aarch64-linux-gnu-ar
OBJCOPY = aarch64-linux-gnu-objcopy
LINKER_SCRIPT = linker.ld
START = start
IMG_NAME = bootloader
BUILD = ../build

CFLAGS := -fno-stack-protector -I ../include/

all: $(BUILD)/$(IMG_NAME).img

$(BUILD)/$(START).o: $(START).S
	$(CC) $(CFLAGS) -c $(START).S -o $(BUILD)/$(START).o

OBJS := \
	bootloader.o \
	uart.o \
	string.o


OBJS := $(addprefix $(BUILD)/, $(OBJS))

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(IMG_NAME).img: $(BUILD)/$(START).o $(OBJS)
	$(LD) $(BUILD)/$(START).o $(OBJS) -T $(LINKER_SCRIPT) -o $(BUILD)/$(IMG_NAME).elf
	$(OBJCOPY) -O binary $(BUILD)/$(IMG_NAME).elf $(BUILD)/$(IMG_NAME).img

clean:
	rm -f $(BUILD)/$(IMG_NAME).elf $(BUILD)/$(IMG_NAME).img $(BUILD)/*.o

run:
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD)/$(IMG_NAME).img -serial null -serial stdio
