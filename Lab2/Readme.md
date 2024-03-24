
kernel backup裡面是"參考"別人的dtb的code，kernel在others的lab2-dtb-kernel8.img，可以直接用但不能用bootloader傳(因為完成時沒有傳x0)，

demo時先de bootloader -> relocate
(傳lab2-basic_and_relocation.img)

再把SD卡改成 有dtb的kernel

待辦: 
弄懂各部分，加上用GDB找bootloader的問題
(真的不行可以clone別人的bootloader來用)

### Relocation
Rpi 3b+ will automatically load kernel into 0x80000, so the bootloader should relocate itself to 0x60000 to preserve the memory for the kernel, and since the linker linked it to 0x60000, we should load the bootloader to 0x60000 for normal performance