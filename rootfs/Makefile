CC 		= clang
OBJCOPY	= llvm-objcopy

CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding \
			  -mcpu=cortex-a53 -mgeneral-regs-only \
			  --target=aarch64-unknown-none-elf \
			  -nostdlib -Os -fPIE

BINS = hello.bin

.PHONY: all clean

all: $(BINS)

clean:
	$(RM) $(BINS)

%.elf: %.ld %.c
	$(CC) $(CFLAGS) -Wl,-T,$^ -o $@

%.elf: %.c
	$(CC) $(CFLAGS) $< -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
