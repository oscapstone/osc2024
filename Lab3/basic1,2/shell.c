#include "uart.h"

struct cpio_newc_header {
    //file metadata
    char c_magic[6];         // Magic number identifying the CPIO format
    char c_ino[8];           // Inode number
    char c_mode[8];          // File mode and type
    char c_uid[8];           // User ID of file owner
    char c_gid[8];           // Group ID of file owner
    char c_nlink[8];         // Number of hard links
    char c_mtime[8];         // Modification time of file
    char c_filesize[8];      // Size of file (in hexadecimal)
    char c_devmajor[8];      // Major device number (for device files)
    char c_devminor[8];      // Minor device number (for device files)
    char c_rdevmajor[8];     // Major device number for the device file node referenced by the symlink
    char c_rdevminor[8];     // Minor device number for the device file node referenced by the symlink
    char c_namesize[8];      // Size of filename (in hexadecimal)
    char c_check[8];         // Checksum
};

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

int hex_to_int(char *p, int len) {
    int val = 0;
    int temp;
    for (int i = 0; i < len; i++) {
        temp = *(p + i);
        if (temp >= 'A') {
            temp = temp - 'A' + 10;
        } else
            temp -= '0';
        val *= 16;
        val += temp;
    }
    return val;
}

void run_user_program(){
    struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000 ;
    char *current = (char *)0x8000000 ;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        
        if (strcmp(current, "user.img") == 0){
            current += name_size;
            if((current - (char *)fs) % 4 != 0)
                current += (4 - (current - (char *)fs) % 4);
            break;
        }

        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
    uart_puts("found user.img\n");
    
    // current is the file address
    asm volatile ("mov x0, 0");  //enable interrupt in el0 (basic1: 0x3c0)
    asm volatile ("msr spsr_el1, x0"); 
    asm volatile ("msr elr_el1, %0": :"r" (current));
    asm volatile ("mov x0, 0x20000");
    asm volatile ("msr sp_el0, x0");
    asm volatile ("eret");
}

void exception_entry() {
    unsigned long spsrel1, elrel1, esrel1;
    asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
    uart_puts("SPSR_EL1: 0x");
    uart_hex_long(spsrel1);
    uart_puts("\n");
    asm volatile ("mrs %0, ELR_EL1" : "=r" (elrel1));
    uart_puts("ELR_EL1: 0x");
    uart_hex_long(elrel1);
    uart_puts("\n");
    asm volatile ("mrs %0, ESR_EL1" : "=r" (esrel1));
    uart_puts("ESR_EL1: 0x");
    uart_hex_long(esrel1);
    uart_puts("\n");
}


void shell(char * cmd){
    if(strcmp(cmd, "help") == 0){
        uart_send('\r');
        uart_puts("Lab3 Exception and Interrupt\n");
        uart_send('\r');
        uart_puts("help\t: print all available commands\n");
        uart_send('\r');
        uart_puts("run\t: set and run user program\n");
    }
    else if(strcmp(cmd, "run") == 0){
        run_user_program();
    }
}

void core_timer_handler() {
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq * 2));
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct));
    cntpct /= cntfrq;

    uart_puts("Seconds since boot: ");
    uart_int(cntpct);
    uart_puts("\n");
}
