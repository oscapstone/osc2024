#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "utils.h"
#include "dtb.h"
#include "memory.h"
#include "timer.h"
#include "sched.h"
#include "syscall.h"
#include "exception.h"

#define CLI_MAX_CMD 14
#define USTACK_SIZE 0x10000

struct CLI_CMDS cmd_list[CLI_MAX_CMD] =
    {
        {.command = "hello", .help = "print Hello World!"},
        {.command = "help", .help = "print all available commands"},
        {.command = "info", .help = "get device information via mailbox"},
        {.command = "cat", .help = "concatenate files and print on the standard output"},
        {.command = "ls", .help = "list directory contents"},
        {.command = "malloc", .help = "simple allocator in heap session"},
        {.command = "dtb", .help = "show device tree"},
        {.command = "exec", .help = "execute a command, replacing current image with a new image"},
        {.command = "setTimeout", .help = "setTimeout [MESSAGE] [SECONDS]"},
        {.command = "set2sAlert", .help = "set core timer interrupt every 2 second"},
        {.command = "memTest", .help = "memory testcase generator, allocate and free"},
        {.command = "thread", .help = "thread tester with dummy function - foo()"},
        {.command = "fork", .help = "fork tester"},
        {.command = "reboot", .help = "reboot the device"}};

void cli_cmd_clear(char *buffer, int length)
{
    for (int i = 0; i < length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char *buffer)
{
    char c = '\0';
    int idx = 0;
    while (1)
    {
        if (idx >= CMD_MAX_LEN)
            break;

        // c = uart_recv();
        c = uart_async_getc();
        if (c == '\n')
        {
            // uart_puts("\r\n");
            break;
        }
        buffer[idx++] = c;
        // uart_send(c);
    }
}

void cli_cmd_exec(char *buffer)
{
    if (!buffer)
        return;

    char *cmd = buffer;
    char *argvs = str_SepbySpace(buffer);
    // char* argvs;

    // while(1){
    //     if(*buffer == '\0')
    //     {
    //         argvs = buffer;
    //         break;
    //     }
    //     if(*buffer == ' ')
    //     {
    //         *buffer = '\0';
    //         argvs = buffer + 1;
    //         break;
    //     }
    //     buffer++;
    // }

    if (strcmp(cmd, "hello") == 0)
    {
        do_cmd_hello();
    }
    else if (strcmp(cmd, "help") == 0)
    {
        do_cmd_help();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        do_cmd_info();
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        do_cmd_cat(argvs);
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        do_cmd_ls(argvs);
    }
    else if (strcmp(cmd, "malloc") == 0)
    {
        do_cmd_malloc();
    }
    else if (strcmp(cmd, "dtb") == 0)
    {
        do_cmd_dtb();
    }
    else if (strcmp(cmd, "exec") == 0)
    {
        do_cmd_exec(argvs);
    }
    else if (strcmp(cmd, "memTest") == 0)
    {
        do_cmd_memory_tester();
    }
    else if (strcmp(cmd, "setTimeout") == 0)
    {
        char *sec = str_SepbySpace(argvs);
        do_cmd_setTimeout(argvs, sec);
    }
    else if (strcmp(cmd, "set2sAlert") == 0)
    {
        do_cmd_set2sAlert();
    }
    else if (strcmp(cmd, "thread") == 0)
    {
        do_cmd_thread_tester();
    }
    else if (strcmp(cmd, "fork") == 0)
    {
        do_cmd_fork_tester();
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        do_cmd_reboot();
    }
    else if (cmd)
    {
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
        uart_puts("Type 'help' to see command list.\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("=======================================\r\n");
    uart_puts("       ------      ------     ------   \r\n");
    uart_puts("     //     //   //         //         \r\n");
    uart_puts("    //     //    ------    //          \r\n");
    uart_puts("   //     //          //  //           \r\n");
    uart_puts("    ------     ------      ------      \r\n");
    uart_puts("   2024 Lab5: Thread and User Process  \r\n");
    uart_puts("=======================================\r\n");
}

void start_shell()
{
    // uart_sendline("In start_shell\n");
    // while(!uart_recv_echo_flag){
    while(list_size(run_queue)>2){
        schedule();
    }
    char input_buffer[CMD_MAX_LEN];
    cli_print_banner();
    uart_recv_echo_flag = 1;
    while (1)
    {
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        // uart_sendline("lock_counter : %d\n",lock_counter);
        uart_puts("Key in command: ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
        // idle();
        // uart_sendline("idle finish\n");
    }
    // return 0;
}

void do_cmd_help()
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
}

void do_cmd_hello()
{
    uart_puts("Hello World!\r\n");
}

void do_cmd_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("Hardware Revision\t: ");
        uart_2hex(pt[6]);
        uart_2hex(pt[5]);
        uart_puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
}

void do_cmd_cat(char *filepath)
{
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if (strcmp(c_filepath, filepath) == 0)
        {
            uart_puts("%s", c_filedata);
            break;
        }

        // if this is TRAILER!!! (last of file)
        if (header_ptr == 0)
            uart_puts("cat: %s: No such file or directory\n", filepath);
    }
}

void do_cmd_ls(char *workdir)
{
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_ptr != 0)
            uart_puts("%s\n", c_filepath);
    }
}

void do_cmd_malloc()
{
    // test malloc
    char *test1 = malloc(0x18);
    memcpy(test1, "test malloc1", sizeof("test malloc1"));
    uart_puts("%s\n", test1);

    char *test2 = malloc(0x20);
    memcpy(test2, "test malloc2", sizeof("test malloc2"));
    uart_puts("%s\n", test2);

    char *test3 = malloc(0x28);
    memcpy(test3, "test malloc3", sizeof("test malloc3"));
    uart_puts("%s\n", test3);
}

void do_cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}

void do_cmd_exec(char *filepath)
{
    char *c_filepath;
    unsigned int c_filesize;
    char *c_filedata;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }
        // core_timer_enable(2);
        if (strcmp(c_filepath, filepath) == 0)
        {
            // exec c_filedata
            /*lab3
            char* ustack = malloc(USTACK_SIZE);
            asm("mov x1, 0x3c0\n\t"
                "msr spsr_el1, x1\n\t" // enable interrupt (PSTATE.DAIF) -> spsr_el1[9:6]=4b0. In Basic#1 sample, EL1 interrupt is disabled.
                "msr elr_el1, %0\n\t"   // elr_el1: Set the address to return to: c_filedata
                "msr sp_el0, %1\n\t"    // user program stack pointer set to new stack.
                "eret\n\t"              // Perform exception return. EL1 -> EL0
                :: "r" (c_filedata),
                   "r" (ustack+USTACK_SIZE));
            break;
            */

            /*lab5*/
            uart_recv_echo_flag = 0; // syscall.img has different mechanism on uart I/O.
            exec_thread(c_filedata, c_filesize);
            // while(1){
            //     schedule();
            // }
            // uart_recv_echo_flag = 1;
        }

        // if this is TRAILER!!! (last of file)
        if (header_ptr == 0)
            uart_puts("exec: %s: No such file or directory\n", filepath);
    }
}

void do_cmd_setTimeout(char *msg, char *sec)
{
    add_timer(uart_sendline, atoi(sec), msg, 0);
}

void do_cmd_set2sAlert()
{
    add_timer(timer_set2sAlert, 2, "2sAlert", 0);
}

void do_cmd_memory_tester()
{

    char *a = kmalloc(0x10000);
    char *aa = kmalloc(0x10000);
    char *aaa = kmalloc(0x10000);
    char *aaaa = kmalloc(0x10000);
    char *b = kmalloc(0x4000);
    char *c = kmalloc(0x1001); // malloc size = 8KB
    char *d = kmalloc(0x10);   // malloc size = 32B
    // char *e = kmalloc(0x800);
    // char *f = kmalloc(0x800);
    // char *g = kmalloc(0x800);

    kfree(a);
    kfree(aa);
    kfree(aaa);
    kfree(aaaa);
    kfree(b);
    kfree(c);
    kfree(d);
}

void do_cmd_thread_tester()
{
    for (int i = 0; i < 5; ++i)
    { // N should > 2
        // uart_sendline("Debug do_cmd_thread_tester i %d\n", i);
        thread_create(foo);
    }
    schedule();

    // add_timer(schedule_timer, 1, "", 0); // start scheduler
    for (int i = 0; i < 3; ++i)
    {
        schedule();
    }
}

/*Not finish: EL1->EL0*/
void fork_test()
{
    // asm volatile("msr tpidr_el1, %0" ::"r"(&curr_thread->context));
    trapframe_t *tpf;
    // asm volatile("mov %0, %1" : "=r"(tpf) : "r"(curr_thread->context.sp));
    asm volatile("mov %0, sp" : "=r"(tpf));
    uart_sendline("Fork test, pid %d\n", getpid(tpf));
    // load_context(&main_thread->context);
    // lock();
    int cnt = 1;
    int ret = 0;
    if ((ret = fork(tpf)) == 0)
    { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_sendline("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(tpf), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork(tpf)) != 0)
        {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_sendline("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(tpf), cnt, &cnt, cur_sp);
        }
        else
        {
            while (cnt < 5)
            {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_sendline("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(tpf), cnt, &cnt, cur_sp);
                int r = 1000000;
                while (r--)
                {
                    asm volatile("nop");
                }
                ++cnt;
            }
        }
        exit(tpf);
        // schedule();
    }
    else
    {
        uart_sendline("parent here, pid %d, child %d\n", getpid(tpf), ret);
        // schedule();
    }
    // unlock();
}
void do_cmd_fork_tester()
{
        lock();
        thread_t *main_thread = thread_create(fork_test);        
        // eret to exception level 0
        // asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
        // "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
        // "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
        // "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process, user program stack pointer set to new stack.
        // "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
        // ::"r"(&main_thread->context),"r"(main_thread->context.lr), "r"(main_thread->stack_allocated_base + USTACK_SIZE), "r"(main_thread->context.sp));
    
        asm("msr elr_el1, %0\n\t" ::"r"(main_thread->context.sp));  // When el0 -> el1, store return address for el1 -> el0 
        asm("msr spsr_el1, xzr\n\t");   // Enable interrupt in EL0 -> Used for thread scheduler
        
        add_timer(schedule_timer, 1, "", 0); // start scheduler
        asm("eret\n\t");
}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}
