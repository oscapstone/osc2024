# `panic-wait`

A panic handler that infinitely waits.

## Usage

Simply import it to the project that needs a `panic_handler` and this crate will set the panic behavior to waiting infinitely.

```rust
#![no_std]

// ...

use panic_wait as _;
```
