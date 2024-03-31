CARGO = cargo
CARGO_FLAGS = --release --target=$(TARGET)

.PHONY: all clean

all: $(KERNEL_ELF) $(BOOTLOADER_ELF)

clean:
	$(CARGO) clean

$(KERNEL_ELF):
	$(CARGO) build $(CARGO_FLAGS)
	