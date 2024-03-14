SRCS = $(wildcard *.c)
OBJ = $(SRCS:.c=.o)
LINKER = linker.ld
CFLAG = -fno-stack-protector -Wall -O2 -ffreestanding -nostartfiles

all: clean kernel8.img

start.o: start.S
		aarch64-linux-gnu-gcc -c start.S -o start.o

%.o: %.c 
		aarch64-linux-gnu-gcc $(CFLAG) -c $< -o $@ 

%.o: %.S  
		aarch64-linux-gnu-gcc $(CFLAG) -c $< -o $@ 

kernel8.img: start.o $(OBJ)
		aarch64-linux-gnu-ld start.o $(OBJ) -T $(LINKER) -o kernel8.elf
		aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

clean: 
		rm kernel8.elf *.o >/dev/null 2>/dev/null  || true

run: kernel8.img
		qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio

