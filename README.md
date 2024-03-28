
# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| cfmc30         | 109350078  | Erh-Tai Huang |

## Requirements

- Install Rust
    ```
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    source $HOME/.cargo/env
    ```

- Tool Chain
    ```
    cargo install cargo-binutils rustfilt
    ```

- Others are specified in [rust-toolchain.toml](rust-toolchain.toml) which will be configured by cargo.

## Things that can be improved

- The structure of the project
- Rust code but with C style
- String with fixed size

## Build

```
make
```

## Test With QEMU

```
make kernel_qemu
```
