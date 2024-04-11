#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "u_string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

char* dtb_ptr;

void main(char* arg){
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);

    uart_init();
    irqtask_list_init(); //創建一個task_list並做初始化
    timer_list_init();   //創建一個timer_event_list並做初始化

    uart_interrupt_enable(); // AUX_MU_IER_REG要初始化，才能將enable interrupt 
    el1_interrupt_enable();  /*這個函式通過清除 DAIF 寄存器的值（將 DAIF 設為0），從而解除對中斷的屏蔽->也就是做enable interrupt
                               這表示在這個函式執行之後，系統將能夠響應中斷。*/
                             // "msr daifclr, 0xf": unmask all DAIF (P.255) 
                             // Used to clear any or all of DAIF to 0 
    core_timer_enable();     // 將CORE0_TIMER_IRQ_CTRL的位置存入2(Address: 0x4000_0040 Core 0 Timers interrupt control)，為了啟用timer!
                             // 0x4000_0040的bit 1(nCNTPNSIRQ)設為1 => 就是將其設為low 32 bits int 2 (w暫存器)

    cli_print_banner();
    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
