OS := $(shell uname)

ifeq ($(OS), Linux)
	ARMGNU  = aarch64-linux-gnu
	MACHINE = raspi3b
endif

ifeq ($(OS), Darwin)
	ARMGNU  = aarch64-unknown-linux-gnu
	MACHINE = raspi3b
endif

CC     = $(ARMGNU)-gcc
LK     = $(ARMGNU)-ld
OBJCPY = $(ARMGNU)-objcopy

A_SRCS = $(wildcard *.S)
C_SRCS = $(wildcard *.c)
OBJS = $(A_SRCS:.S=.o) $(C_SRCS:.c=.o)
USR_FILE = ../rootfs/$(notdir $(basename $(OBJS)))

CFLAGS = -Wno-implicit -ffreestanding -nostdlib -nostartfiles

all: $(USR_FILE)
$(USR_FILE): $(OBJS)
	$(LK) $(OBJS) -T linker.ld -o $(notdir $(USR_FILE)).elf
	$(OBJCPY) -O binary $(notdir $(USR_FILE)).elf $(USR_FILE)

%.o: %.S
	$(CC) -c $< $(CFLAGS) -o $@

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf $(USR_FILE) $(OBJS) *.elf