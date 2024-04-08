#include "uart.h"
#include "irq.h"
#include "timer.h"

#define BUFFER_SIZE 1024
#define MAX_TIMER 10

unsigned long timer_times[MAX_TIMER];
char timer_msgs[MAX_TIMER][1024];
unsigned int timer_idx;

char uart_read_buffer[BUFFER_SIZE];
unsigned int read_idx = 0;

char uart_write_buffer[BUFFER_SIZE];
unsigned int write_idx = 0;
unsigned int write_cur = 0;
int async;

void uart_read_handler() {
    char ch = (char)(*AUX_MU_IO);
    if(ch == '\r'){ //command
        //uart_send('\n');
        uart_read_buffer[read_idx] = '\0';
        //uart_puts(uart_read_buffer);
        uart_write_buffer[write_idx] = '\n';
        write_idx++;
        shell(uart_read_buffer);
        read_idx = 0;
        uart_read_buffer[read_idx] = '\0';
    }
    else{
        uart_read_buffer[read_idx] = ch;
        uart_write_buffer[write_idx] = ch; 
        //uart_send(uart_read_buffer[read_idx]);
        read_idx++;
        write_idx++;
    }
}

void uart_write_handler(){
    if(write_cur < write_idx){
        *AUX_MU_IO=uart_write_buffer[write_cur];
        write_cur++;
    }
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
    asm volatile ("mov x0, 0x345"); 
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
    async = 1;
    uart_interrupt();
    while(1){}
}

int add_timer(unsigned long cur_time, char *s){
    for(int i=0; i<MAX_TIMER; i++){
        if(timer_times[i] == 0){
            timer_times[i] = cur_time;
            int j = 0;
            while(*s != '\0'){
                timer_msgs[i][j] = *s;
                s++;
                j++;
            }
            timer_msgs[i][j] = '\0';
            //save message
            return i; //allocated timer
        }
    }
    return -1;
}

void add_message_timer(){
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
    if(wait == 0){
        uart_puts("INVALID TIME\n");
        return;
    }
    cur_time = get_current_time();
    print_time = cur_time + wait;
    
    int allocated = add_timer(print_time, message);
    if(allocated < 0){
        uart_puts("timer busy");
        return;
    }
    int new_irq = 1;
    unsigned long min_time = timer_times[allocated];
    for(int i=0; i<MAX_TIMER; i++){
        if(timer_times[i]<min_time && timer_times[i] > 0){
            new_irq = 0;
            break;
        }
    }

    if(new_irq){
        uart_puts("New Interrupt Set in timer ");
        uart_int(allocated);
        uart_puts("\n");
        set_timer_interrupt(print_time - cur_time);
        //asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq));
    }
    uart_puts("Seconds to print: ");
    uart_int(print_time);
    uart_puts("\n");
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
    }
    else if(strcmp(cmd, "run") == 0){
        run_user_program();
    }
    else if(strcmp(cmd, "async") == 0){
        async_uart_io();
    }
    else if(strcmp(cmd, "setTimeout") == 0 || strcmp(cmd, "st") == 0){
        add_message_timer();
    }
    else
        return 0;
    return 1;
}

void timer_handler() {
    unsigned long cur_time = get_current_time();
    unsigned long next = 9999;
    for(int i=0;i<MAX_TIMER;i++){
        if(timer_times[i] == cur_time){
            uart_puts("\n[TIMER] ");
            uart_int(cur_time);
            uart_puts(" : ");
            uart_puts(timer_msgs[i]);
            uart_puts("\n");
            uart_send('\r');
            uart_puts("# ");
            timer_times[i] = 0;
        }
        else if(timer_times[i] > cur_time){
            if(next > timer_times[i])
                next = timer_times[i];
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
    int irq_pending1 = *IRQ_PENDING_1;
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & 2){
        timer_handler();
        //timer_handler();
    }
    else{
        if ((iir & 0x06) == 0x04)
            uart_read_handler();
        else
            uart_write_handler();
    }
    //asm volatile("msr DAIFClr, 0xf");
}
