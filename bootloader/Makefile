CARGO = cargo
CARGO_FLAGS = --release --target=$(TARGET)

.PHONY: all clean

all: $(BOOTLOADER_ELF)

clean:
	$(CARGO) clean

$(BOOTLOADER_ELF):
	$(CARGO) build $(CARGO_FLAGS)
	