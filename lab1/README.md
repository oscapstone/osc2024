# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| maxwang1104 | 310552061    | 王俊崴 |


## Files
| File          | Content                                               | 
| --------------| ----------------------------------------------------- | 
| command.c(.h) | code for action to deal with different shell command  |
| gpio.c        | some gpio config                                      |
| main.c        | main program                                          |
| math.c(.h)    | code for replace standard math.h                      |
| shell.c(.h)   | code for control the shell behave                     |
| string.c(.h)  | code for replace standard math.h                      |
| uart.c(.h)    | code for uart opertaion                               |
| link.ld       | linker script                                         |


## How to build

```bash
make
```

## Run on QEMU
```bash
make run
```

## Simple Shell
| command   | description                   | 
| ----------| ----------------------------- | 
| hello     | print Hello World!            |
| help      | print all available commands  |
| reboot    | reset rpi3                    |
| info      | info of revision and ARM memory.                  |
