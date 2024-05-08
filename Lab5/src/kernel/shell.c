#include "kernel/shell.h"
#include <stddef.h>
void gdb_break(){
    uart_puts("this is just for gdb\n");
}

int boot_timer_flag = 1;

void my_shell(){
    char buf[MAX_BUF_LEN];
    char *argv[5];
    int buf_index;
    //char input_char;
    for(buf_index = 0; buf_index < 5; buf_index++)
        argv[buf_index] = simple_malloc(MAX_ARGV_LEN);

    copy_process(PF_KTHREAD, (my_uint64_t)&idle_process, 0, 0);
    current_task = PCB[0];
    // tpidr_el1 hold a thread id
    asm volatile("msr tpidr_el1, %0" ::"r" (&(current_task->context))); /// malloc a space for current kernel thread to prevent crash
    

    while(1){
        buf_index = 0;
        string_set(buf, 0, MAX_BUF_LEN);
        uart_puts("# ");

        buf_index = uart_gets(buf, argv);
        //buf_index = uart_get_fn(buf);
        // this one requires to tuen on interrupt in main function(can move to somewhere else in the future)
        //buf_index = uart_irq_gets(buf);

        if(buf_index >= MAX_BUF_LEN)
            uart_puts("Warning: buffer is full, command output may not correct\n");

        if(!string_comp(buf, "help")){
            uart_puts("help     :Print this help menu\n");
            uart_puts("hello    :Print Hello World!\n");
            uart_puts("info     :Get revision and memory\n");
            uart_puts("reboot   :Reboot the device\n");
            uart_puts("ls       :list all files in initramfs\n");
            uart_puts("cat      :show the content of file\n");
            uart_puts("aloc     :allocate a string\n");
            uart_puts("el0      :execute programs in initramfs in El0\n");
            uart_puts("async    :try the async I/O\n");
            uart_puts("off      :turn off the 2 seconds timer\n");
            uart_puts("settimeout: set the timeout for the task\n");
            uart_puts("timer    :run the timer interrupt test\n");
            uart_puts("task     :run the task test\n");
            uart_puts("thread   :run the thread test\n");
        }
        else if(!string_comp(buf, "hello")){
            uart_puts("Hello World!\n");
        }
        else if(!string_comp(buf, "info")){
            //uart_puts("Mailbox function is still woring on\n");
            /*if(mailbox_call()){
                get_board_revision();
                uart_puts("My board revision is: ");
                uart_b2x(mailbox[5]);
                uart_puts("\r\n");
            }*/
            get_board_revision();
            get_arm_mem();
        }
        else if(!string_comp(buf, "reboot")){
            //uart_puts("Reboot function is still woring on\n");
            uart_puts("Rebooting...\n");
            // after 1000 ticks, start resetting
            reset(1000);
        }
        else if(!string_comp(buf, "ls")){
            cpio_ls();
        }
        else if(!string_comp(buf, "cat")){
            //uart_puts("cat is still working on\n");
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Filename: ");
            
            buf_index = uart_gets(buf, argv);
            if(buf_index >= MAX_BUF_LEN)
                uart_puts("Warning: buffer is full, command output may not correct\n");
        
            cpio_cat(buf);

            continue;
        }
        else if(!string_comp(buf, "aloc")){
            char* string = pool_alloc(8);

            string[0] = 'S';
            string[1] = 't';
            string[2] = 'r';
            string[3] = 'i';
            string[4] = 'n';
            string[5] = 'g';
            string[6] = '!';
            string[7] = '\0';
            uart_puts(string);
            uart_putc('\n');
            show_mem_stat();
        }
        else if(!string_comp(buf, "el0")){
            void *file_addr;
            void *stack_addr;
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Program name: ");

            buf_index = uart_gets(buf, argv);
            if(buf_index >= MAX_BUF_LEN)
                uart_puts("Warning: buffer is full, command output may not correct\n");

            file_addr = cpio_find(buf);
            // indicating that the file is either a directory or not exist
            if(file_addr == 0)
                continue;

            stack_addr = simple_malloc(2048);
            // https://stackoverflow.com/questions/47516089/how-do-i-access-local-c-variable-in-arm-inline-assembly
            asm volatile(
                "mov x1, 0x3c0;"
                "msr spsr_el1, x1;"
                "mov x1, %[var1];"
                "mov x2, %[var2];"
                "msr elr_el1, x1;"
                "msr sp_el0, x2;"
                "eret"                         
                :                                                   // Output operand: empty here
                : [var1] "r" (file_addr),[var2] "r" (stack_addr)    // Input operand
                : "x1", "x2"                                        // clobbered registers(those are modified)
            );
            gdb_break();
        }
        else if(!string_comp(buf, "async")){
            char async_buf[MAX_BUF_LEN];
            int ticks = 150;
            int_on();
            uart_irq_on();

            uart_irq_puts("Async I/O test:");
            uart_irq_gets(async_buf);
            uart_irq_puts("You just typed:");
            uart_irq_puts(async_buf);
            // if this line is not added, the output will be mixed with the next command
            while(ticks--);

            uart_irq_off();
        }
        else if(!string_comp(buf, "off")){
            if(boot_timer_flag != 0){
                boot_timer_flag = 0;
            }
        }
        else if(!string_comp(buf, "settimeout")){
            uart_puts(argv[0]);
            uart_putc(' ');
            uart_puts(argv[1]);
            //uart_b2x(h2i(argv[1], string_len(argv[1])));

            settimeout(argv[0], h2i(argv[1], string_len(argv[1])));
        }
        else if(!string_comp(buf, "timer")){
            unsigned long long cur_cnt, cnt_freq;

            asm volatile(
                "mrs %[var1], cntpct_el0;"
                "mrs %[var2], cntfrq_el0;"
                :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
            );

            uart_puts("Current Time:");
            uart_b2x_64(cur_cnt / cnt_freq);

            settimeout("task1", 6);
            settimeout("task2", 3);
            settimeout("task3", 9);
        }
        else if(!string_comp(buf, "task")){
            task_create_DF1(print_callback, "task1", 1);
            task_create_DF1(print_callback, "task2", 2);
            task_create_DF0(task_callback, 0);

            ExecTasks();
        }
        else if(!string_comp(buf, "mem")){
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(196);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
            pool_alloc(196);
            pool_alloc(4096);
        }
        else if(!string_comp(buf, "buddy")){
            //buddy_init();
            //memory_reserve((void*)0x10003A28, (void*)0x10003A28 + 0x1000);
            uart_puts("-----------------\n");
            void *a1 = buddy_malloc(4096);
            uart_puts("-----------------\n");
            void *a12 = buddy_malloc(4096);
            uart_puts("-----------------\n");
            void *a13 = buddy_malloc(4096);
            uart_puts("-----------------\n");
            void *a8 = buddy_malloc(8192);
            uart_puts("-----------------\n");
            void *a82 = buddy_malloc(8193);
            uart_puts("-----------------\n");
            uart_puts("start free\n");
            buddy_free(a82);
            uart_puts("-----------------\n");
            buddy_free(a1);
            uart_puts("-----------------\n");
            buddy_free(a12);
            uart_puts("-----------------\n");
            buddy_free(a13);
            uart_puts("-----------------\n");
            buddy_free(a8);
            uart_puts("End free");
            uart_puts("-----------------\n");
            pool_alloc(16);
            uart_puts("-----------------\n");
            pool_alloc(16);
            uart_puts("-----------------\n");
            pool_free(pool_alloc(10));
            uart_puts("-----------------\n");
            pool_free(pool_alloc(1025));
            uart_puts("-----------------\n");
        }
        else if(!string_comp(buf, "thread")){
            int i = 0;
            thread_init();

            for(; i < 5; i++)
                thread_create(foo, 0);
            // start scheduling
            asm volatile("msr tpidr_el1, %0" ::"r" (pool_alloc(sizeof(thread_t))));
            idle_task();
        }
        else if(!string_comp(buf, "test")){
            test_NI = h2i(argv[0], string_len(argv[0]));

            uart_itoa(test_NI);
            uart_putc('\n');

            uart_irq_on();
            delay(1000);
            uart_puts("Press a key to test nested interrupt\n");
            
            PRI_TEST_FLAG = 0;
            while(PRI_TEST_FLAG == 0){
                if(PRI_TEST_FLAG == 1)
                    break;
            }
            delay(1000);

            uart_irq_off();
            test_NI = 0;
        }
        else if(!string_comp(buf, "syscall")){
            thread_init();
            thread_create(fork_test, 0);
            idle_task();
        }
        else if(!string_comp(buf, "process")){
            int_on();
            uart_irq_on();
            //uart_itoa(sizeof(buddy_block_list_t));
            //uart_putc('\n');
            // uart_b2x_64((my_uint64_t)&idle_process);
            // uart_putc('\n');
            // uart_b2x_64((my_uint64_t)&kernel_procsss);
            // uart_putc('\n');
            //int res = copy_process(PF_KTHREAD, (my_uint64_t)&idle_process, 0, 0);
            int res2 = copy_process(PF_KTHREAD, (my_uint64_t)&kernel_procsss, 0, 0);
            if(res2 < 0){
                uart_puts("Create process failed\n");
                continue;
            }
            //idle_process();
            //while(1)
                process_schedule();
        }
        else if(!string_comp(buf, "test2")){
            void *file_addr;
            int file_name_len;
            char file_name[50];
            uint64_t tmp; 

            asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
            tmp |= 1;
            asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
            
            uart_puts("Program name: ");

            file_name_len = uart_irq_gets(file_name);
            if(file_name_len >= 50)
                uart_puts("Warning: buffer is full, command output may not correct\n");

            file_addr = cpio_find(file_name);
            // indicating that the file is either a directory or not exist
            if(file_addr == 0)
                continue;

            uart_b2x_64((my_uint64_t)file_addr);
            uart_putc('\n');

            int_on();
            uart_irq_on();

            int res = copy_process(PF_KTHREAD, (my_uint64_t)&file_process, (my_uint64_t)file_addr, 0);
            if(res < 0){
                uart_puts("Create process failed\n");
                continue;
            }
            // uart_puts("Start schedule\n");
            process_schedule();
        }
        else{
            uart_puts("Unknown Command: ");
            uart_puts(buf);
        }
    }
}