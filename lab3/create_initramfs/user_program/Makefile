# ARMGNU ?= aarch64-linux-gnu
ARMGNU ?= aarch64-none-elf

PROGRAM_NAME ?= example

SRC := $(PROGRAM_NAME).S
OBJ := $(PROGRAM_NAME).o
ELF := $(PROGRAM_NAME).elf
IMG := $(PROGRAM_NAME).img

CC := $(ARMGNU)-gcc
LD := $(ARMGNU)-ld
OBJCOPY := $(ARMGNU)-objcopy

ASMFLAGS := -c

all: $(IMG)

$(OBJ): $(SRC)
	$(CC) $(ASMFLAGS) -o $(OBJ) $(SRC)

$(ELF): $(OBJ)
	$(LD) -T link.ld -o $(ELF) $(OBJ)

$(IMG): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(IMG)

clean:
	rm -rf $(OBJ) $(ELF) $(IMG)