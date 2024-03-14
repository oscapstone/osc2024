# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| SamKao-writer  | 310551131  | Shih-Hsuan Kao|

# Requirements

- Cross compiler: aarch64-linux-gnu-gcc
- Emulator: qemu-system-aarch64

# Build

```
make kernel8.img
```

# Test with QEMU
```
make qe
(qemu-system-aarch64 -M raspi3b -kernel $< -serial null -serial stdio -display none)
```
