#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "string.h"
#include "dtb.h"
#include "memory.h"
#include "timer.h"
#include "sched.h"
#include "colourful.h"
#include "vfs.h"
#include "exception.h"
#include "syscall.h"
#include "initramfs.h"


#define CLI_MAX_CMD 9

extern int   uart_recv_echo_flag;
extern char* dtb_ptr;
extern void* CPIO_DEFAULT_START;
extern void* _kernel_start;
int cmd_list_size = 0;
// struct vnode *current_dir;
char current_dir[30] = "/";

struct CLI_CMDS cmd_list[] =
{
    {.command="help",                   .func=do_cmd_help,          .help="print all available commands"},
    {.command="exec",                   .func=do_cmd_exec,          .help="execute a command, replacing current image with a new image"},
    {.command="vfs_tester",             .func=do_cmd_vfs_tester,    .help="VFS tester"},
    {.command="cat",                    .func=do_cmd_cat,           .help="concatenate files and print on the standard output"},
    {.command="mkdir",                  .func=do_cmd_mkdir,         .help="make a new directory"},
    {.command="ls",                     .func=do_cmd_ls,            .help="list directory contents"},
    {.command="cd",                     .func=do_cmd_cd,            .help="change directory"},
    {.command="pwd",                    .func=do_cmd_pwd,           .help="show the current directory"},
    {.command="read",                   .func=do_cmd_read,          .help="read the file"},
    {.command="write",                  .func=do_cmd_write,         .help="write something to the file"},


    {.command="ls_cpio",                .func=do_cmd_ls_cpio,       .help="list CPIO directory contents"},
    {.command="hello",                  .func=do_cmd_hello,         .help="print Hello World!"},
    {.command="dtb",                    .func=do_cmd_dtb,           .help="show device tree"},
    {.command="info",                   .func=do_cmd_info,          .help="get device information via mailbox"},
    {.command="thread_tester",          .func=do_cmd_thread_tester, .help="thread tester with dummy function - foo()"},
    {.command="setTimeout",             .func=do_cmd_setTimeout,    .help="setTimeout [MESSAGE] [SECONDS]"},
    {.command="memory_tester",          .func=do_cmd_memory_tester, .help="memory testcase generator, allocate and free"},
    {.command="reboot",                 .func=do_cmd_reboot,        .help="reboot the device"},
    {.command="c",                      .func=do_cmd_cancel_reboot, .help="cancel reboot the device"}
};
void cli_cmd_init()
{
    cmd_list_size = sizeof(cmd_list) / sizeof(struct CLI_CMDS);
}
void cli_cmd()
{
    cli_print_banner();
    char input_buffer[CMD_MAX_LEN];
    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts(GRN "( %s )" RESET "# ", current_dir);
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
void cli_cmd_clear(char* buffer, int length)
{
    for(int i=0; i<length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char* buffer)
{
    char c = '\0';
    int idx = 0;
    while(1)
    {
        // uart_sendline("idx: %d\r\n", idx);
        if ( idx >= CMD_MAX_LEN ) break;
        c = uart_async_getc();

        // if user key 'enter'
        if ( c == '\n')
        {
            uart_puts("\r\n");
            buffer[idx] = '\0';
            break;
        }

        // if user key 'backspace'
        if ( c == '\b' || c == 127 )
        {
            if ( idx > 0 )
            {
                uart_puts("\b \b");
                idx--;
                buffer[idx] = '\0';
            }
            continue;
        }

        // use tab to auto complete
        if ( c == '\t' )
        {
            for(int tab_index = 0; tab_index < cmd_list_size; tab_index++)
            {
                if (strncmp(buffer, cmd_list[tab_index].command, strlen(buffer)) == 0)
                {
                    for (int j = 0; j < strlen(buffer); j++)
                    {
                        uart_puts("\b \b");
                    }
                    uart_puts(cmd_list[tab_index].command);
                    cli_cmd_clear(buffer, strlen(buffer) + 3);
                    strcpy(buffer, cmd_list[tab_index].command);
                    idx = strlen(buffer);
                    break;
                }
            }
            continue;
        }

        // some ascii blacklist
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;

        buffer[idx++] = c;
        // uart_send(c); // we don't need this anymore

    }
}

void cli_cmd_exec(char* buffer)
{
    if (!buffer) return;

    char *words[3] = {NULL, NULL, NULL};
    char sep = ' ';
    // int argc = str_SepbySpace(buffer, words) - 1;
    int argc = str_SepbySomething(buffer, words, sep) - 1;

    char* cmd       = words[0];
    char* argvs[2]  = {words[1], words[2]};
    // argvs[0] = words[1];
    // argvs[1] = words[2];

    for (int i = 0; i < cmd_list_size; i++)
    {
        if (strcmp(cmd, cmd_list[i].command) == 0)
        {
            cmd_list[i].func(argvs, argc);
            return;            
        }
    }
    if (*cmd != '\0')
    {
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts(BLU "=======================================\r\n" RESET);
    uart_puts(RED "  OSC 2024 Lab7 Shell  \r\n" RESET);
    uart_puts(BLU "=======================================\r\n" RESET);
}

DO_CMD_FUNC(do_cmd_cat)
{
    char* filepath = argv[0];
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;
        if(strcmp(c_filepath, filepath)==0)
        {
            uart_puts("%s", c_filedata);
            break;
        }
        if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
    }
    return 0;
}

DO_CMD_FUNC(do_cmd_thread_tester)
{
    int num_thread = 5;
    if (argv[0] != NULL){
        num_thread = atoi(argv[0]);
    }
    uart_sendline("%d Threads Testing ...\r\n", num_thread);
    
    for (int i = 0; i < num_thread; ++i)
    {
        thread_create(foo, 0, NORMAL_PRIORITY);
    }
    schedule();
    uart_puts("%d Thread tester done! \r\n", num_thread);
    return 0;
}

DO_CMD_FUNC(do_cmd_dtb)
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
    return 0;

}
DO_CMD_FUNC(do_cmd_memory_tester)
{
    frame_t *frame_array = get_frame_array();
    
    char *p2 = kmalloc(0x900);
    char *p3 = kmalloc(0x2000);
    char *p4 = kmalloc(0x3900);
    // uart_sendline("p1: %x, p2: %x, p3: %x, p4: %x\r\n", p1, p2, p3, p4);
    kfree(p3);
    kfree(p4);
    // kfree(p1);
    kfree(p2);

    char *p[10];
    for (int i = 0; i < 10; i++)
    {
        p[i] = kmalloc(0x1000);
    }

    for (int i = 0; i < 10;i++)
    {
        kfree(p[i]);
    }

    char *a = kmalloc(0x10);        // 16 byte
    char *b = kmalloc(0x100);
    char *c = kmalloc(0x1000);

    kfree(a);
    kfree(b);
    kfree(c);

    a = kmalloc(32);
    char *aa = kmalloc(50);
    b = kmalloc(64);
    char *bb = kmalloc(64);
    c = kmalloc(128);
    char *cc = kmalloc(129);
    char *d = kmalloc(256);
    char *dd = kmalloc(256);
    char *e = kmalloc(512);
    char *ee = kmalloc(999);

    char *f = kmalloc(0x2000);
    char *ff = kmalloc(0x2000);
    char *g = kmalloc(0x2000);
    char *gg = kmalloc(0x2000);
    char *h = kmalloc(0x2000);
    char *hh = kmalloc(0x2000);

    kfree(a);
    kfree(aa);
    kfree(b);
    kfree(bb);
    kfree(c);
    kfree(cc);
    kfree(dd);
    kfree(d);
    kfree(e);
    kfree(ee);

    kfree(f);
    kfree(ff);
    kfree(g);
    kfree(gg);
    kfree(h);
    kfree(hh);
    char *p1 = kmalloc(0x4000);
    uart_sendline("p1: %x\r\n", p1);
    int frame_idx = PTR_TO_PAGE_INDEX(VIRT_TO_PHYS(p1));
    uart_sendline("frame_idx: %d\r\n", frame_idx);
    frame_t *frame = &frame_array[frame_idx];
    uart_sendline("frame-val: %d\n", frame->val);
    for (int i = 0; i <= frame->val; i++){
        frame_t *cur = &frame_array[frame_idx + i];
        uart_sendline("frame->counter: %d\n", cur->counter);
    }
    kfree(p1);
    for (int i = 0; i <= frame->val; i++){
        frame_t *cur = &frame_array[frame_idx + i];
        uart_sendline("frame->counter: %d\n", cur->counter);
    }
    return 0;
}
DO_CMD_FUNC(do_cmd_help)
{
    for(int i = 0; i < cmd_list_size; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
    return 0;

}

DO_CMD_FUNC(do_cmd_exec)
{
    char abs_path[MAX_PATH_NAME] = "";
    if (argv[0] == NULL)
    {
        uart_puts("Usage: exec [FILENAME]\r\n");
        return -1;
    }
    strcpy(abs_path, argv[0]);
    get_absolute_path(abs_path, current_dir);

    struct vnode *target = NULL;
    vfs_lookup(abs_path, &target);
    if (target == NULL)
    {
        uart_puts("exec: %s: No such file or directory\n", abs_path);
        return -1;
    }

    // strcpy(abs_path, "/initramfs/vfs1.img");

    uart_sendline("executing %s ...\r\n", abs_path);
    uart_recv_echo_flag = 0;
    thread_exec_vfs(abs_path);

    return 0;
}

// DO_CMD_FUNC(do_cmd_exec)
// {
//     char* filepath = argv[0];
//     char* c_filepath;
//     char* c_filedata;
//     unsigned int c_filesize;
//     struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

//     filepath = "vfs1.img";
//     // filepath = "vm.img";
//     while(header_ptr!=0)
//     {
//         int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
//         if(error) break;

//         if(strcmp(c_filepath, filepath)==0)
//         {
//             uart_sendline("executing %s ...\r\n", filepath);
//             uart_recv_echo_flag = 0; // syscall.img has different mechanism on uart I/O.
//             thread_exec(c_filedata, c_filesize);
//             break;
//         }
//         if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
//     }
//     return 0;
// }

DO_CMD_FUNC(do_cmd_hello)
{
    uart_puts("Hello World!\r\n");
    return 0;
}
DO_CMD_FUNC(do_cmd_info)
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
    return 0;
}


DO_CMD_FUNC(do_cmd_ls_cpio)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;
        if(header_ptr!=0) uart_puts("%s\n", c_filepath);
    }
    return 0;
}


DO_CMD_FUNC(do_cmd_ls)
{
    char abs_path[MAX_PATH_NAME];
    if (argv[0] == NULL)
    {
        strcpy(abs_path, current_dir);
    }
    else{
        strcpy(abs_path, argv[0]);
        get_absolute_path(abs_path, current_dir);
    }
    vfs_list_dir(abs_path);

    return 0;
}


DO_CMD_FUNC(do_cmd_mkdir)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, argv[0]);

    if (abs_path == NULL)
    {
        uart_puts("Usage: mkdir [DIRNAME]\r\n");
        return 0;
    }
    get_absolute_path(abs_path, current_dir);

    vfs_mkdir(abs_path);

    return 0;
}

DO_CMD_FUNC(do_cmd_cd)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, argv[0]);
    if (abs_path == NULL)
    {
        uart_puts("Usage: cd [DIRNAME]\r\n");
        return -1;
    }

    get_absolute_path(abs_path, current_dir);

    if (vfs_cd(abs_path) == 0)
    {
        strcpy(current_dir, abs_path);
    }

    return 0;
}

DO_CMD_FUNC(do_cmd_pwd)
{
    uart_puts(current_dir);
    uart_puts("\r\n");
    return 0;
}

DO_CMD_FUNC(do_cmd_read)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, argv[0]);
    if (abs_path == NULL)
    {
        uart_puts("Usage: write [FILENAME] [MESSAGE]\r\n");
        return -1;
    }
    get_absolute_path(abs_path, current_dir);

    trapframe_t *tpf = kmalloc(sizeof(trapframe_t));

    tpf->x0 = (uint64_t)abs_path;
    tpf->x1 = O_CREAT;
    int fd = sys_open(tpf);

    char buf[100];
    tpf->x0 = fd;
    tpf->x1 = (uint64_t)buf;
    tpf->x2 = 100;
    sys_read(tpf);
    uart_puts(buf);

    tpf->x0 = fd;
    sys_close(tpf);
    
    
    kfree(tpf);
    return 0;
}

DO_CMD_FUNC(do_cmd_write)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, argv[0]);
    if (abs_path == NULL)
    {
        uart_puts("Usage: write [FILENAME] [MESSAGE]\r\n");
        return -1;
    }
    get_absolute_path(abs_path, current_dir);

    trapframe_t *tpf = kmalloc(sizeof(trapframe_t));
    
    tpf->x0 = (uint64_t)abs_path;
    tpf->x1 = O_CREAT;
    int fd = sys_open(tpf);


    char message[MAX_PATH_NAME];
    strcpy(message, argv[1]);
    int len = strlen(message);
    message[len] = '\n';
    message[len+1] = '\0';
    if (message == NULL)
    {
        uart_puts("Usage: write [FILENAME] [MESSAGE]\r\n");
        return -1;
    }

    tpf->x0 = fd;
    tpf->x1 = (uint64_t)message;
    tpf->x2 = strlen(message);
    sys_write(tpf);
    
    tpf->x0 = fd;
    sys_close(tpf);
    
    
    kfree(tpf);
    return 0;

}


DO_CMD_FUNC(do_cmd_setTimeout)
{
    char* msg = argv[0];
    char* sec = argv[1];

    if (msg == NULL || sec == NULL)
    {
        uart_puts("Usage: setTimeout [MESSAGE] [SECONDS]\r\n");
        return 0;
    }
    add_timer(uart_sendline,atoi(sec),msg,0);
    return 0;
}

DO_CMD_FUNC(do_cmd_reboot)
{
    // uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    char* kernel_start = (char*) (&_kernel_start);
    uart_sendline("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;

    unsigned long long expired_tick = 10 * 10000;

    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = (unsigned long long)PM_PASSWORD | expired_tick;
    ((void (*)(char*))kernel_start)(dtb_ptr);
    return 0;
}

DO_CMD_FUNC(do_cmd_cancel_reboot)
{
    uart_puts("Cancel Reboot \r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x0;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0;
    return 0;
}
DO_CMD_FUNC(do_cmd_vfs_tester)
{
    vfs_test();
    return 0;
}

