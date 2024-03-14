GNUARM = aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude

SRC_C = $(wildcard src/*.c) # find all .c file
SRC_S = $(wildcard *.S) # find all .S file
OBJS = $(SRC_C:.c=.o) # replace all .c in SRC_C with .o by using %.o:%.c
OBJS += $(SRC_S:.S=.o) # replace all .S in SRC_S with .o by using %.o:%.S

all: kernel8.img

%.o: %.S
	$(GNUARM)-gcc $(CFLAGS) -c $< -o $@

%.o: %.c
	$(GNUARM)-gcc $(CFLAGS) -c $< -o $@

kernel8.img: linker.ld $(OBJS)
	$(GNUARM)-ld -T linker.ld -o kernel8.elf $(OBJS)
	$(GNUARM)-objcopy -O binary kernel8.elf kernel8.img

qemu:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -S -s

clean:
	rm *.o src/*.o kernel8.*