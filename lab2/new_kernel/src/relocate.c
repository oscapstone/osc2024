// void code_relocate(char* addr)
// {
//     unsigned long long size = (unsigned long long)&__code_size;
//     char* start = (char *)&_start;
//     for(unsigned long long i=0;i<size;i++)
//     {
//         addr[i] = start[i];
//     }

//     ((void (*)(char*))addr)(_dtb);
// }

// extern unsigned char __begin, __end, __boot_loader_addr;

// __attribute__((section(".text.relocate"))) void relocate() {
//     unsigned long kernel_size = (&__end - &__begin);
//     unsigned char *new_bl = (unsigned char *)&__boot_loader_addr;
//     unsigned char *bl = (unsigned char *)&__begin;

//     while (kernel_size--) {
//         *new_bl++ = *bl;
//         *bl++ = 0;
//     }

//     void (*start)(void) = (void *)&__boot_loader_addr;
//     start();
//     /* `start()` 函式是一個函式指標，指向 `__boot_loader` 地址。在這段程式碼中，
//     `__boot_loader` 被假設為一個函式的起始地址。當 `start()` 函式被調用時，它會跳轉到 
//     `__boot_loader` 所指向的位置，開始執行該函式。 在這個特定的例子中，
//     `relocate()` 函式最後一行 `start();` 會將控制權轉移給 
//     `__boot_loader` 所指向的函式，這樣程序就會開始執行 `__boot_loader` 函式中的指令。*/
// }