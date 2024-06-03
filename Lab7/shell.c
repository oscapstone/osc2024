#include "uart.h"
#include "irq.h"
#include "timer.h"
#include "dtb.h"
#include "vfs.h"

#define BUFFER_SIZE 1024
#define MAX_TIMER 10

typedef void (*timer_callback_t)(char* data);

struct timer{
    timer_callback_t callback;
    char data[BUFFER_SIZE];
    unsigned long expires;
};

struct timer timers[MAX_TIMER];

// unsigned long timer_times[MAX_TIMER];
// char timer_msgs[MAX_TIMER][1024];
// unsigned int timer_idx;

char uart_read_buffer[BUFFER_SIZE];
unsigned int read_idx = 0;

char uart_write_buffer[BUFFER_SIZE];
unsigned int write_idx = 0;
unsigned int write_cur = 0;

char async_cmd[BUFFER_SIZE];

void test_daif(){
    unsigned long spsrel1;
    uart_puts("Before test\n");
    asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
    uart_puts("SPSR_EL1: 0x");
    uart_hex_long(spsrel1);
    uart_puts("\n");
    int r;
    asm volatile("msr DAIFSet, 0xf");
    uart_puts("in test\n");
    asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
    uart_puts("SPSR_EL1: 0x");
    uart_hex_long(spsrel1);
    uart_puts("\n");
    asm volatile("msr DAIFClr, 0xf");
    uart_puts("After test\n");
    asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
    uart_puts("SPSR_EL1: 0x");
    uart_hex_long(spsrel1);
    uart_puts("\n");
    uart_puts("--------------------------------\n");
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
    char ch = '\0';
    if(write_cur < write_idx){
        *AUX_MU_IO=uart_write_buffer[write_cur]; // i think need interrupt here so cannot asm
        ch = uart_write_buffer[write_cur];
        write_cur++;
        *AUX_MU_IER |= (0x01);
        *AUX_MU_IER |= (0x02);
    }
    else{
        *AUX_MU_IER |= (0x01);
        //*AUX_MU_IER &= ~(0x02);
    }
    
    if(ch == '\r'){
        shell(async_cmd);
        write_cur = 0;
        write_idx = 0;
    }
}

void uart_read_handler() {
    char ch = (char)(*AUX_MU_IO);
    if(ch == '\r'){ //command
        //uart_send('\n');
        uart_read_buffer[read_idx] = '\0';
        read_idx = 0;
        strcpy(uart_read_buffer, async_cmd);
        //uart_puts(uart_read_buffer);
        uart_write_buffer[write_idx] = '\n';
        write_idx++;
        uart_write_buffer[write_idx] = '\r';
        write_idx++;
    }
    else{
        uart_read_buffer[read_idx] = ch;
        uart_write_buffer[write_idx] = ch; 
        read_idx++;
        write_idx++;
    }
    *AUX_MU_IER |= 0x01; //start
    *AUX_MU_IER |= 0x02;
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

int str2int(char* s) {
    int result = 0;
    int sign = 1; // This is to handle negative numbers

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
    split_line();
    uart_puts("In run user program\n");
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base ;
    char *current = (char *)cpio_base ;
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
    asm volatile ("mov x0, 0"); 
    asm volatile ("msr spsr_el1, x0"); 
    asm volatile ("msr elr_el1, %0": :"r" (current));
    asm volatile ("mov x0, 0x20000");
    asm volatile ("msr sp_el0, x0");
    asm volatile ("eret");
}

void exception_entry() {
    split_line();
    uart_puts("In exception\n");
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
    while(1){}
}

void async_uart_io(){
    //uart_interrupt();
    while(1){}
}

void add_timer(timer_callback_t callback, char* data, unsigned long after){
    asm volatile("msr DAIFSet, 0xf");
    int i;
    int allocated = 0;
    unsigned long cur_time = get_current_time();
    unsigned long print_time = cur_time + after;
    for(i=0; i<MAX_TIMER; i++){
        if(timers[i].expires == 0){
            timers[i].expires = print_time;
            int j = 0;
            while(*data != '\0'){
                timers[i].data[j] = *data;
                data++;
                j++;
            }
            timers[i].data[j] = '\0';
            //uart_puts("timer data: ");
            //uart_puts(timers[i].data[j]);
            timers[i].callback = callback;
            allocated = 1;
            break;
        }
    }

    if(allocated == 0){
        uart_puts("Timer Busy\n");
    }

    int new_irq = 1;
    unsigned long min_time = print_time;
    for(int i=0; i<MAX_TIMER; i++){
        if(timers[i].expires < min_time && timers[i].expires > 0){
            new_irq = 0;
            break;
        }
    }

    if(new_irq){
        set_timer_interrupt(print_time - cur_time);
    }
    uart_puts("Seconds to print: ");
    uart_int(print_time);
    uart_puts("\n");
    asm volatile("msr DAIFClr, 0xf");
}


void setTimeout_callback(char* data) {
    // Convert data back to the appropriate type and print the message
    uart_puts(data);
    uart_puts("\n");
    uart_send('\r');
    uart_puts("# ");
}

void setTimeout_cmd(){
    uart_puts("MESSAGE: ");
    char in_char;
    char message[100];
    int idx = 0;
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            message[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            message[idx] = in_char;
            idx++;
        }
    }
    uart_puts("SECONDS: ");
    idx = 0;
    char countDown[100];
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            countDown[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            countDown[idx] = in_char;
            idx++;
        }
    }
    unsigned long cur_time, print_time;
    int wait = str2int(countDown);
    if(wait <= 0){
        uart_puts("INVALID TIME\n");
        return;
    }
    
    add_timer(setTimeout_callback, message, wait);
}

void cpio_ls(){
    uart_send('\r');
    uart_puts("");
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    char *current = (char *)cpio_base;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (strcmp(current, "TRAILER!!!") == 0)
            break;

        uart_puts(current);
        uart_puts("\n");
        uart_send('\r');
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
}

void cpio_cat(){
    uart_send('\r');
    uart_puts("Filename: ");
    char in_char;
    char filename[100];
    int idx = 0;
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            filename[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            filename[idx] = in_char;
            idx++;
        }
    }
    
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    char *current = (char *)cpio_base;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (strcmp(current, "TRAILER!!!") == 0){
            uart_send('\r');
            uart_puts(filename);
            uart_puts(": No such file.\n");
            break;
        }
        if (strcmp(current, filename) == 0){
            current += name_size;
            if((current - (char *)fs) % 4 != 0)
                current += (4 - (current - (char *)fs) % 4);
            uart_send('\r');
            uart_puts(current);
            uart_puts("\n");
            break;
        }
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
}


int shell(char * cmd){
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
        uart_puts("cpio_ls\t: list all files\n");
        uart_send('\r');
        uart_puts("cat\t: show content of file\n");
    }
    else if(strcmp(cmd, "cpio_ls") == 0){
        cpio_ls();
    }
    else if(strcmp(cmd, "cat") == 0){
        cpio_cat();
    }
    else if(strcmp(cmd, "run") == 0){
        run_user_program();
    }
    else if(strcmp(cmd, "async") == 0){
        async_uart_io();
    }
    else if(strcmp(cmd, "setTimeout") == 0 || strcmp(cmd, "st") == 0){
        setTimeout_cmd();
    }
    else if(strcmp(cmd, "test") == 0){
        test_daif();
    }
    else if(strcmp(cmd, "pwd") == 0){
        pwd();
    }
    else if(strcmp(cmd, "ls") == 0){
        vfs_ls("/");
    }
    else
        return 0;
    return 1;
}

void timer_handler() {
    unsigned long cur_time = get_current_time();
    unsigned long next = 9999;
    for(int i=0;i<MAX_TIMER;i++){
        if(timers[i].expires == cur_time){
            uart_puts("\n[TIMER] ");
            uart_int(cur_time);
            uart_puts(" : ");
            timers[i].callback(timers[i].data);
            timers[i].expires = 0;
        }
        else if(timers[i].expires > cur_time){
            if(next > timers[i].expires)
                next = timers[i].expires;
        }
    }
    disable_core_timer();
    if(next != 9999){
        set_timer_interrupt(next - cur_time);
        //uart_puts("resetted another timer\n");
    }
}

void interrupt_handler_entry(){
    //asm volatile("msr DAIFSet, 0xf"); add here will boom, check how to add
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & 2){
        timer_handler();
    }
    else{
        *AUX_MU_IER &= ~(0x01);
        *AUX_MU_IER &= ~(0x02);
        if ((iir & 0x06) == 0x04){
            uart_read_handler();
        }
        else{
            uart_write_handler();
        }
    }
    
    //asm volatile("msr DAIFClr, 0xf");
}