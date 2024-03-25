# Lab 0

## How to boot on rpi board

When power on

- First-stage bootloader on ROM
  - CPU/RAM are not initialized
  - GPU handles this first stage bootloader
- Second-stage bootloader
  - Mount `bootcode.bin` on FAT32 of SD card
  - GPU places `bootcode.bin` on its L2 cache, activates RAM and read `start.elf`
- GPU firmware
  - Start.elf (third-stage bootloader)
    - VideoCore OS
    - Read `config.txt` that represents BIOS setting
    - GPU and CPU RAM use different memory region
  - Run `fixup.dat`
    - Organize SDRAM partition between GPU/CPU
    - Reset CPU
  - Read zImage to RAM and kernel takes over
