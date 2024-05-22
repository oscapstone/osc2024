# NYCU Operating System Capstone 2024

## Dependancy
- `make`
- `rust`
- `qemu` (optional)
- `picocom` (optional)
- `gcc` (optional)

## Develop Environment

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup component add rust-src --toolchain nightly-x86_64-unknown-linux-gnu
rustup component add llvm-tools-preview
cargo install cargo-binutils
rustup component add llvm-tools-preview
```


## Build & Run
- Make bootloader, kernel image and initramfs etc.
    ```sh
    make
    ```

- Run the bootloader with daemonized qemu.
    ```sh
    make run
    ```
    You will see a serial port, like `/dev/pts/<number>`

- Attach to that serail port via picocom with baud rate set to 115200.
    ```sh
    picocom -b 115200 /dev/pts/<number>
    ```

- Open another terminal and send the built kernel image to the bootloader via `cat`.
    ```sh
    cat build/kernel8.img > /dev/pts/<number>
    ```
- Remember to stop qemu since its executing in the background.
