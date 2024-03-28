CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
CFLAGS := -Wall -Wextra -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -IMain -IUART/header -IShell/header -IMailbox/header -IUtils/header -IReboot/header
CFLAGS += -I/usr/lib/gcc-cross/aarch64-linux-gnu/11/include
K8 := kernel8
LINKER := Main/linker.ld

SRC_S := $(wildcard Main/*.s)
SRC_C := $(wildcard Main/*.c UART/*.c Shell/*.c Mailbox/*.c Reboot/*.c Utils/*.c)
OBJS := $(SRC_C:.c=.o) $(SRC_S:.s=.o) Main/main.o

all: $(K8).img

$(K8).img: $(OBJS)
	$(LD) -T $(LINKER) -o $(K8).elf $^
	$(OBJCOPY) -O binary $(K8).elf $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

sd: $(K8).img
	cp $(K8).img /media/ben/4DFF-0A36
	sync

qemu: 
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel $(K8).img

rpi3:
	screen /dev/ttyUSB0 115200

clean:
	rm -f $(wildcard */*.o) $(wildcard *.o) $(K8).elf $(K8).img
