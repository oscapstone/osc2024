
# LAB 1

this is the lab to set up the environment and access the hardware with own image

## Basic Exercise 1 - Basic Initialization - 20%

### Edit the code for booting the system

only use a single cpu core for simplicity
we need to read the register MPIDR_EL1 (64 bit reg) and access the lowest 2 bit to halt all other cores other than one



ref:
* https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/Multiprocessor-Affinity-Register
