# Lab5
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab5.html)
---
## Basic Exercises
### Basic Exercise 1 - Thread
+ background

### Basic Exercise 2 - User Process and System Call 
+ Background

+ system call -> save_all -> exception handler -> create process -> save trap_frame of the caller process into pt_regs -> scedule ->  -> use switch_to by loading new process 'context' struct to update x19 ~ x31 and pc -> load_all -> eret -> jump to the new loaded context.lr and use context.sp

+ TPIDR_EL1: Provides a location where software executing at EL1 can store thread identifying information, for OS management purposes.

+ Remember that the load_all and save_all must perfectly align with trap frame

### Basic Exercise 3 - Video Player

## Advanced Exercises
### Advanced Exercise 1 - POSIX Signal 
+ Background
+ I should already implemented basic exercise in O(logn) as I access the metadata in order level instead of iterate all indexs.