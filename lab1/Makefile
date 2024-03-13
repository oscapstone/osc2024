CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g
SRCS = $(shell find . -name '*.c')
OBJS = $(patsubst ./%.c,obj/%.o,$(SRCS))

all: clean kernel8.img

$(OBJS): obj/%.o : %.c
	mkdir -p $(dir $@)
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

obj/start.o: start.S
	mkdir -p $(dir $@)
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

kernel8.img: obj/start.o $(OBJS)
	aarch64-elf-ld -nostdlib $^ -T linker.ld -o kernel8.elf
	aarch64-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -rf obj kernel8.elf kernel8.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s -S

# 创建obj目录及其子目录
obj:
	mkdir -p obj
