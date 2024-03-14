.PHONY: all clean run

SOURCE=a.S
OBJ=$(SOURCE:.S=.o)
ELF=kernel8.elf
IMG=$(ELF:.elf=.img)
LINKER=linker.ld

all: $(IMG) $(OBJ)

$(OBJ): $(SOURCE)
	aarch64-linux-gnu-gcc -o $@ -c $^

$(ELF): $(OBJ) $(LINKER)
	aarch64-linux-gnu-ld -T $(LINKER) -o $@ $<

$(IMG): $(ELF)
	aarch64-linux-gnu-objcopy -O binary $^ $@

run: $(IMG)
	qemu-system-aarch64 -M raspi3b -kernel $^ -display none -d in_asm

clean:
	rm $(OBJ) $(ELF) $(IMG)
