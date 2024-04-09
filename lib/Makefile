CC = clang
CFLAGS = -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc -Iinclude -MMD
TARGET = --target=aarch64-rpi3-elf -mcpu=cortex-a53

SRC = src
OBJ = obj

SRC_FILES = $(wildcard $(SRC)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC)/%.c=$(OBJ)/%.o)
DEP_FILES = $(OBJ_FILES:%.o=%.d)


.PHONY: all
all: libab.a

libab.a: $(OBJ_FILES)
	@ar -rv $@ $^ 

-include $(DEP_FILES)

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(TARGET) -c $< -o $@


.PHONY: clean
clean:
	-@rm -v libab.a
	-@rm -rv $(OBJ)
	







