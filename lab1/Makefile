
CC 		= clang
LD 		= ld.lld
OBJ_CPY		= llvm-objcopy
LINK_SCRIPT 	= link.ld 
CFLAGS 		= --target=aarch64-elf -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53+nosimd

SRC_DIR		= src
INCLUDE_DIR 	= include
OUT_DIR		= obj

SRCS 		= $(wildcard $(SRC_DIR)/*.c )
OBJS 		= $(patsubst $(SRC_DIR)/%.c, %.o, $(SRCS))

VPATH = $(SRC_DIR)
vpath %.c %.S $(SRC_DIR) 

kernel8.img: start.o $(OBJS)
	$(LD) -m aarch64elf -nostdlib $(OUT_DIR)/start.o $(addprefix $(OUT_DIR)/,$(notdir $(OBJS))) -T $(LINK_SCRIPT) -o kernel8.elf
	$(OBJ_CPY) -O binary kernel8.elf $@

all: kernel8.img

start.o: start.S
	$(CC) $(CFLAGS) -c $< -o $(OUT_DIR)/start.o

%.o: %.c
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $< -o $(OUT_DIR)/$@

# Make Command
run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

clean:
	rm kernel8.elf kernel8.img obj/*.o *.o >/dev/null 2>/dev/null || true

debug:	
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s -serial null -serial stdio