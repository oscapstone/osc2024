#include "kernel/uart.h"
#include "kernel/INT.h"
#include "kernel/gpio.h"
#include "kernel/timer.h"
#include "kernel/task.h"

void c_exception_handler(){
    void *spsr1;
    void *elr1;
    void *esr1;
    void *el;

    uart_puts("Entering exception handler\n");

    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        "mrs %[var3], esr_el1;"
        "mrs %[var4], CurrentEL;"
        : [var1] "=r" (spsr1),[var2] "=r" (elr1),[var3] "=r" (esr1),[var4] "=r" (el)    // Output operands
    );

    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("SPSR_EL1:  ");
    uart_b2x_64((unsigned long long)spsr1);
    uart_putc('\n');

    uart_puts("ELR_EL1:   ");
    uart_b2x_64((unsigned long long)elr1);
    uart_putc('\n');

    uart_puts("ESR_EL1:   ");
    uart_b2x_64((unsigned long long)esr1);
    uart_putc('\n');

    uart_puts("Leaving exception handler\n");
    // It will keep printing as next line of boot.S is 'b exception_handler'
    /*while(1){
        asm volatile("nop");
    }*/
}

void c_core_timer_handler(){
    unsigned long long cur_cnt, cnt_freq, el;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        "mrs %[var3], CurrentEL;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq), [var3] "=r" (el)
    );
    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("Time after boots: ");
    uart_b2x_64(cur_cnt / cnt_freq);
    uart_puts(" sec.\n");

    cnt_freq *= 2;

    /*asm volatile(
        "msr cntp_tval_el0, %[var1];"
        "msr spsr_el1, %[var2];"
        :
        :[var1] "r" (cnt_freq), [var2] "r" (0)
        :
    );*/
    asm volatile(
        "msr cntp_tval_el0, %[var1];"
        :
        :[var1] "r" (cnt_freq)
        :
    );
}

void c_write_handler(){
    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);

    while(write_index_cur != write_index_tail){
        char c = write_buffer[write_index_cur++];

        // If no CR, first line of output will be moved right for n chars(n=shell command just input), not sure why
        if(c == '\n'){
            while(!((*AUX_MU_LSR_REG) & 0x20) ){
                asm volatile("nop");
            }
            *AUX_MU_IO_REG = '\r';
        }

        // check if FIFO can accept at least one byte after sending one character
        while(!((*AUX_MU_LSR_REG) & 0x20) ){
            // if bit 5 is set, break and return IO_REG
            asm volatile("nop");
        }
        *AUX_MU_IO_REG = c;
        
        write_index_cur = write_index_cur % MAX_BUF_LEN;
    }
    // enable receiver interrupt
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | (0x1));
}

void c_recv_handler(){
    while(!((*AUX_MU_LSR_REG) & 0x01) ){
        // if bit 0 is set, break and return IO_REG
        asm volatile("nop");
    }

    char c = (char)(*AUX_MU_IO_REG);
    //uart_putc(c);
    c = (c=='\r'?'\n':c);

    read_buffer[read_index_tail++] = c;
    read_index_tail = read_index_tail % MAX_BUF_LEN;

    // Put into write buffer such that the received char can be echoed out
    write_buffer[write_index_tail++] = c;
    write_index_tail = write_index_tail % MAX_BUF_LEN;

    //task_create_DF0(c_write_handler, 2);
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);
}

void c_timer_handler(){
    unsigned long long cur_cnt, value;
    task_timer_t *cur = timer_head;

    if(cur == 0)
        return;

    value = 0;
    // disable interrupt to protect critical section
    asm volatile(
        "msr cntp_ctl_el0, %[var1];"
        :
        :[var1] "r" (value)
    );

    asm volatile(
        "msr daifset, 0xf;"
    );

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        :[var1] "=r" (cur_cnt)
    );
    //uart_b2x_64(cur_cnt);
    //uart_putc('\n');

    while(cur_cnt >= cur->deadline){
        cur->callback(cur->data);
        
        timer_head = cur->next;
        if(timer_head != 0){
            timer_head->prev = 0;
            // If the timeout is earlier than the previous programed expired time, the kernel reprograms the hardware timer to the earlier one.
            // enable timer
            value = 1;
            asm volatile(
                "msr cntp_cval_el0, %[var1];"
                "msr cntp_ctl_el0, %[var2];"
                :
                :[var1] "r" (timer_head->deadline), [var2] "r" (1)
            );
            // Since we turn off the core0 timer interrupt in general interrupt handler, we need to turn it on again
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);
        }
        else{
            uart_puts("No more timer\n");
            // disable timer
            value = 0;
            asm volatile(
                "msr cntp_ctl_el0, %[var1];"
                :
                :[var1] "r" (value)
            );

            break;
        }
        // free cur(not implemented)
        cur = cur->next;
    }

    // enable all interrupt of current EL
    asm volatile(
        "msr daifclr, 0xf;"
    );
}

void c_general_irq_handler(){
    unsigned int cpu_irq_src, gpu_irq_src;
    unsigned long long spsr1, elr1, el;
    // First save interrupt current status, then turn off interrupt by mask DAIF bits
    /*asm volatile(
        "mrs %[var1], daif;"
        "msr daifset, 0xf;"
        :[var1] "=r" (daif)
    );*/

    asm volatile(
        "mrs %[var1], CurrentEL;"
        :[var1] "=r" (el)
    );
    
    /*uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');*/

    cpu_irq_src = mmio_read((long)CORE0_INT_SRC);

    /*uart_puts("core0 src:");
    uart_b2x(cpu_irq_src);
    uart_putc('\n');*/
    
    // Through this, we can see that uart interrupt is in pending register 1(0x00000100)
    /*irq_src = mmio_read((long)IRQ_basic_pending);
    uart_puts("basic pending:");
    uart_b2x(irq_src);
    uart_putc('\n');*/

    gpu_irq_src = mmio_read((long)IRQ_pending_1);
    /*uart_puts("pending 1:");
    uart_b2x(gpu_irq_src);
    uart_putc('\n');*/
    // There periphial interrupt in pending 1 register
    // if bit29, meaning a async write or read
    if(gpu_irq_src & (1 << 29)){
        //uart_puts("Periphial IRQ\n");

        unsigned int irq_status = mmio_read((long)AUX_MU_IIR_REG);
        //uart_b2x(irq_status);
        // [2:1]=10 : Receiver holds valid byte 
        if(irq_status & 0x4){
            // disable receive interrupt by setting bit1 to 0
            mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));
            task_create_DF0(c_recv_handler, 1);
            //c_recv_handler();
            // not enable interrupt as we must first let recv_handler to execute
            //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | (0x1));
        }
        // [2:1]=01 : Transmit holding register empty
        if(irq_status & 0x2){
            // disable transmit interrupt, set bit2 to 0
            mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x2));
            //c_write_handler();
            task_create_DF0(c_write_handler, 0);
        }
    }
    // CNTPNSIRQ interrupt bit, this is by observation, not quite sure why is that bit(which is Non-secure physical timer event.)
    // https://developer.arm.com/documentation/100964/1118/Fast-Models-components/SystemIP-components/GIC-400
    if(cpu_irq_src & (0x1 << 1)){
        uart_puts("Timer IRQ\n");

        if(boot_timer_flag != 0){
            c_core_timer_handler();
        }
        else{
            // disable core0 timer interrupt, 
            // p.13 https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 0);
            c_timer_handler();
        }
    }
    // Restore interrupt status
    /*asm volatile(
        "msr daif, %[var1];"
        :
        :[var1] "r" (daif)
    );*/
    // Save the current state
    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        : [var1] "=r" (spsr1), [var2] "=r" (elr1)
    );
    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
    ExecTasks();
    // Restore the previous state
    asm volatile(
        "msr spsr_el1, %[var1];"
        "msr elr_el1, %[var2];"
        :
        : [var1] "r" (spsr1), [var2] "r" (elr1)
    );
    /*while(1){

    }*/
}