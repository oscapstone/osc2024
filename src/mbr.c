#include "sd.h"
#include "mbr.h"
#include "string.h"
#include "uart.h"

struct general_mbr mbr;

void get_mbr(void)
{
    char buf[512];

    readblock(0, buf);
    memcpy(&mbr, buf, BLOCK_SIZE);
    if (mbr.boot_signature != MBR_BOOT_SIG) {
        printf("[get_mbr] Can't get MBR\n");
        while (1);
    }
}