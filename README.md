# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| HelloHomework  | 312551018  | Tsung-Che Lu  |

```
rustc --version
rustc 1.78.0-nightly (4a0cc881d 2024-03-11)
aarch64-unknown-none-softfloat
```

RUSTFLAGS="-C target-cpu=cortex-a53 -C link-arg=--library-path=src/ -C link-arg=--script=src/bsp/rpi3/kernel.ld" cargo rustc --target=aarch64-unknown-none-softfloat 
rust-objcopy --strip-all -O binary target/aarch64-unknown-none-softfloat/debug/kernel kernel8.img
echo kernel8.img