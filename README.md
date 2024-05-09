# OSC2024 Lab3

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
|    nien-zhu    | 312515040  |  Nien-Chu Yu  |

## Equipment

<table>
    <tr>
        <th colspan="2">Raspberry Pi 3B+ v1.4</th> 
    </tr>
    <tr>
        <td align="center">CPU</td>
        <td>BCM2837B0</td>
    </tr>
    <tr>
        <td>Microarchitecture</td>
        <td>Cortex-A53</td>
    </tr>
    <tr>
        <td>Max memory capacity</td>
        <td>1GB</td>
    </tr>
    <tr>
        <td>Revision</td>
        <td>a020d4</td>
    </tr>
</table>

## Requirements

* cross-compiler: aarch64-linux-gnu-gcc
* Emulator: qemu-system-aarch64

## Build 

```
make
```

## Debug 

```
make test
make gdb
```

## Test With QEMU

```
make run 
(qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img)
```

