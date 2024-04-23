#include "uart.h"
#include "irq.h"
#include "timer.h"
#include "task_queue.h"

#define BUFFER_SIZE 1024

// unsigned long timer_times[MAX_TIMER];
// char timer_msgs[MAX_TIMER][1024];
// unsigned int timer_idx;

char uart_read_buffer[BUFFER_SIZE];
unsigned int read_idx = 0;

char uart_write_buffer[BUFFER_SIZE];
unsigned int write_idx = 0;
unsigned int write_cur = 0;

char async_cmd[BUFFER_SIZE];

static char *cpio_base;

void initramfs_callback(char *address)
{
    cpio_base = address;
}

void strcpy(char *s1, char *s2){
    while(*s1){
        *s2 = *s1;
        s1++;
        s2++;
    }
    *s2 = '\0';
}

void uart_write_handler(){
    //*AUX_MU_IER |= 0x02;
    //uart_puts("hi\n");
    //*AUX_MU_IER &= ~(0x02);
    //asm volatile("msr DAIFSet, 0xf");
    char ch = '\0';
    if(write_cur < write_idx){
        *AUX_MU_IO=uart_write_buffer[write_cur]; // i think need interrupt here so cannot asm
        ch = uart_write_buffer[write_cur];
        write_cur++;
    }
    else{
        *AUX_MU_IER &= ~(0x02);
    }
    
    if(ch == '\r'){
        shell(async_cmd);
        write_cur = 0;
        write_idx = 0;
    }
    //asm volatile("msr DAIFClr, 0xf");
    //*AUX_MU_IER &= ~(0x02);
    //*AUX_MU_IER |= 0x02;
}

void uart_read_handler() {
    //asm volatile("msr DAIFSet, 0xf");
    *AUX_MU_IER &= ~(0x01); //stop read interrupt
    char ch = (char)(*AUX_MU_IO);
    if(ch == '\r'){ //command
        //uart_send('\n');
        uart_read_buffer[read_idx] = '\0';
        read_idx = 0;
        strcpy(uart_read_buffer, async_cmd);
        //uart_puts(uart_read_buffer);
        uart_write_buffer[write_idx] = '\n';
        write_idx++;
        create_task(uart_write_handler,2);
        uart_write_buffer[write_idx] = '\r';
        write_idx++;
        //shell(uart_read_buffer);
        //uart_read_buffer[read_idx] = '\0';
    }
    else{
        uart_read_buffer[read_idx] = ch;
        uart_write_buffer[write_idx] = ch; 
        //uart_send(uart_read_buffer[read_idx]);
        read_idx++;
        write_idx++;
    }
    *AUX_MU_IER |= 0x01; //start
    *AUX_MU_IER |= 0x02;
    //asm volatile("msr DAIFClr, 0xf");
    //create_task(uart_write_handler,2);
}

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

long str2int(char* s) {
    long result = 0;
    long sign = 1; // This is to handle negative numbers

    // Check for negative number
    if (*s == '-') {
        sign = -1;
        s++; // Move to the next character
    }

    while (*s) {
        if (*s < '0' || *s > '9') {
            // Handle error: not a digit
            return 0; // Or some error code
        }
        result = result * 10 + (*s - '0');
        s++;
    }

    return result * sign;
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
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base; //switch between qemu and rpi
    char *current = (char *)cpio_base;
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
    uart_hex(current);
    uart_send('\n');
    // current is the file address
    asm volatile ("mov x0, 0x3c0"); 
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

void async_uart_io(){
    uart_interrupt();
    while(1){}
}


int parse_cmd(char * cmd){
    char * run = cmd;
    char name[100];
    char msg[100];
    char time[100];
    int idx = 0;
    while(*run){
        if(*run == ' '){
            name[idx] = '\0';
            run++;
            break;
        }
        name[idx] = *run;
        run++;
        idx++;
    }

    if(!*run)
        return -1;
    
    if(strcmp(name, "st") == 0 || strcmp(name, "setTimeout") == 0){
    }
    else
        return -1;
    
    idx = 0;
    while(*run){
        if(*run == ' '){
            msg[idx] = '\0';
            run++;
            break;
        }
        msg[idx] = *run;
        run++;
        idx++;
    }
    
    if(!*run)
        return -1;

    idx = 0;
    while(*run){
        time[idx] = *run;
        run++;
        idx++;
    }
    time[idx] = '\0';

    unsigned long wait = str2int(time);
    if(wait <= 0){
        uart_puts("\rINVALID TIME\n");
        return 1;
    }
    
    add_timer(setTimeout_callback, msg, wait);
    return 1;
}

int shell(char * cmd){
    if(parse_cmd(cmd) == 1){
        return 1;
    }
    if(strcmp(cmd, "help") == 0){
        uart_send('\r');
        uart_puts("Lab3 Exception and Interrupt\n");
        uart_send('\r');
        uart_puts("help\t: print all available commands\n");
        uart_send('\r');
        uart_puts("run\t: set and run user program\n");
        uart_send('\r');
        uart_puts("st\t: set a timer interrupt\n");
        uart_send('\r');
        uart_puts("async\t: start uart interrupt\n");
    }
    else if(strcmp(cmd, "run") == 0){
        run_user_program();
    }
    else if(strcmp(cmd, "async") == 0){
        uart_send('\r');
        async_uart_io();
    }
    else if(strcmp(cmd, "setTimeout") == 0 || strcmp(cmd, "st") == 0){
        setTimeout_cmd();
    }
    else if(strcmp(cmd, "buf") == 0){
        show_buffer();
    }
    else
        return 0;
    return 1;
}

void interrupt_handler_entry(){
    //asm volatile("msr DAIFSet, 0xf"); add here will boom, check how to add

    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & 2){
        create_task(timer_handler, 3);
        // timer_handler();
    }
    else{
        if ((iir & 0x06) == 0x04){
            create_task(uart_read_handler,1);
        }
        else
            create_task(uart_write_handler,2);
    }
    execute_task();
    //asm volatile("msr DAIFClr, 0xf");
}
