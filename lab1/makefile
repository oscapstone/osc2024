CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g 
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ASMS = $(wildcard *.S)
ASM_OBJS = $(ASMS:.S=.o)

all:: clean_img flash clean

%.o: %.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@


%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

flash: $(ASM_OBJS) $(OBJS)
	aarch64-linux-gnu-ld $(ASM_OBJS) $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
clean:
	rm -f $(ASM_OBJS) $(OBJS)
	
clean_img:
	rm -f kernel8.elf
	rm -f kernel8.img
test:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s
