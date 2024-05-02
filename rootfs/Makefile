CC 		= clang
OBJCOPY	= llvm-objcopy

CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding \
			  -mcpu=cortex-a53 -mgeneral-regs-only \
			  --target=aarch64-unknown-none-elf \
			  -nostdlib -Os -fPIE \
			  -I../include/lib -g

BINS = hello.img svc.img syscall.img fork.img

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS)

%.elf: %.ld %.c
	$(CC) $(CFLAGS) -Wl,-T,$^ -o $@

%.elf: %.c
	$(CC) $(CFLAGS) $< -o $@

%.elf: %.s
	$(CC) $(CFLAGS) $< -o $@

%.img: %.elf
	$(OBJCOPY) -O binary $< $@

syscall.img:
	curl https://nycu-caslab.github.io/OSC2024/_downloads/58c515e3041658a045033c8e56ecff4c/initramfs.cpio --output - | cpio -id
