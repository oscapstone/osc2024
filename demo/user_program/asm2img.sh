aarch64-linux-gnu-as -o user_program.o user_program.S
aarch64-linux-gnu-ld -o user_program.elf user_program.o
aarch64-linux-gnu-objcopy -O binary user_program.elf -O binary ../rootfs/userprogram.img