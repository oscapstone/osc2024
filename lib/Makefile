CC = clang
CFLAGS = -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc -Iinclude
TARGET = --target=aarch64-rpi3-elf -mcpu=cortex-a53

SRC = src
OBJ = obj

SRCFILES = $(wildcard $(SRC)/*.c)
OBJFILES = $(SRCFILES:$(SRC)/%.c=$(OBJ)/%.o)


.PHONY: all
all: libab.a

libab.a: $(OBJFILES)
	@ar -rv $@ $^

$(OBJ)/%.o: $(SRC)/%.c $(OBJ)
	$(CC) $(CFLAGS) $(TARGET) -c $< -o $@

$(OBJ):
	mkdir -p $@

.PHONY: clean
clean:
	-@rm -v libab.a
	-@rm -rv $(OBJ)
	







