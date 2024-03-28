// #include "shell.h"
#include "dtb.h"
#include "stdint.h"


void kernel_main(char* x0) {
    dtb_init(x0);
    shell();
}