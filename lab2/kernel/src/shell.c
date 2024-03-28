#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "utils.h"
#include "cpio.h"
#include "heap.h"
#include "dtb.h"
#include <stddef.h>

#define CLI_MAX_CMD 8

extern char* dtb_ptr;
unsigned long CPIO_DEFAULT_PLACE;

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
    {.command = "help", .help = "print all available commands", .func = do_cmd_help},
    {.command = "info", .help = "get device information via mailbox", .func = do_cmd_info},
    {.command = "reboot", .help = "reboot the device", .func = do_cmd_reboot},
    {.command = "c", .help = "cancel reboot", .func = do_cmd_reboot_cancel},
    {.command = "ls", .help="list directory contents", .func = do_cmd_ls},
    {.command = "cat", .help = "concatenate files and print on the standard output", .func = do_cmd_cat},
    {.command = "malloc", .help = "simple allocator in heap session", .func = do_cmd_malloc},
    {.command = "dtb", .help = "show device tree", .func = do_cmd_dtb}
};

void cli_cmd_clear(char* buffer, int length)
{
    for(int i=0; i<length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char* buffer)
{
    char c='\0';
    int idx = 0;
    while(1)
    {
        if ( idx >= CMD_MAX_LEN ) break;

        c = uart_recv();
        if ( c == '\n')
        {
            uart_puts("\r\n");
            break;
        }
        if ( c == 127 )
        {
            if ( idx == 0) continue;
            uart_puts("\b \b");
            idx --;
            buffer[idx] = '\0';            
            continue;
        }
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;
        buffer[idx++] = c;
        uart_send(c);
    }
}

void cli_cmd_exec(char *buffer)
{
    if (!buffer) return;

    char* cmd = buffer;
    char* argvs;

    while(1){
        if(*buffer == '\0')
        {
            argvs = buffer;
            break;
        }
        if(*buffer == ' ')
        {
            *buffer = '\0';
            argvs = buffer + 1;
            break;
        }
        buffer++;
    }

    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        if (strcmp(cmd, cmd_list[i].command) == 0)
        {
            cmd_list[i].func();
            return;            
        }
        if (strcmp(cmd, "cat") == 0) {
       	    do_cmd_cat(argvs);
       	    return;
        }
    }
    if (*cmd)
    {
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts("============================================================================================\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%@@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-@@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@# =@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@= :@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-  %@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%   *@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#%@@@@@@@@@@@@@@@@@@@*   -@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@% *@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@:=@@@@@@@@@@@@@@@@@@@:    %@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@- :@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*  #@@@@@@@@@@@@@@@@@*     -@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@#-   :#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%+    +%@@@@@@@@@@@@@@+       -%@@@@@@@@@@@@\r\n");
    uart_puts("@@@@*=:                                                 :=#@@@@@@@@#=:          =*@@@@@@@@@@\r\n");
    uart_puts("@@@@@#=                                                =*%@@@@@@@@@@*+-       :=*%@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@-      @@@@@@@@@@@#    -@@@@@@@@@@@@@@@@@@%:  -%@%@@@@@@@@@@@@@@%=   :#@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@=      @%++*%@@@@@%:   +@@@@#==#@@@@@@@@@@@%  %*  -%@@#-=+%@@@@@@@-  %@@@@@+::=*%@@@@\r\n");
    uart_puts("@@@@@@@=      @%                        =%@+               #@+                          =%@@\r\n");
    uart_puts("@@@@@@@=      @%      ************-      =@                 #+                          :%@@\r\n");
    uart_puts("@@@@@@@=      @%      @@@@@@@@@@@@+      @@%    #=          +=      %@@@*   =@@@@-     :@@@@\r\n");
    uart_puts("@@@@@@@=      @%      ##########%@+     :@@%    #=    +@@@@@@=      %@#=     -#@@=     -@@@@\r\n");
    uart_puts("@@@@@@@=      @%      :::::::::::@+     :@@%    #=    +#+=++@=      -:         -:      -@@@@\r\n");
    uart_puts("@@@@@@@=      @%      @@@@@@@@@@@@+     :@@%    #=    ++    @=      -           :      -@@@@\r\n");
    uart_puts("@@@@@@@=     :@%      @@@@@@@@@@@@+     :@@%    #=    ++    @=      %@#-     :*%@-     -@@@@\r\n");
    uart_puts("@@@@@@@=     =@%                        :@@%    #=    ++    @=      %@@@+   -@@@@-     -@@@@\r\n");
    uart_puts("@@@@@@@-     #@%     ****:     +**=     +@@%    #=    ++    @=      *####:  #####:     -@@@@\r\n");
    uart_puts("@@@@@@@     :@@%-=*%@@@@@-     #@@#+*#%@@@@%    #=    ++   =@+                         -@@@@\r\n");
    uart_puts("@@@@@@*     %@@@@#*=--@@@-     #@@#-=+*%@@@%    %=    *+  =@@+     +@@@@@= :@@@@@#     -@@@@\r\n");
    uart_puts("@@@@@@:    *@%+-      =@@-     #@#:     :=*+   :@=    +*=%@@@+   =%@@@@@*   -@@@@@@+:  -@@@@\r\n");
    uart_puts("@@@@@=    *#-           *-     #-              *@=    +@@@@@@#+*@@@@@@*-     :+%@@@@@#+*@@@@\r\n");
    uart_puts("@@@@+   -%@:     -=**%@@@-     #@@%#*+=:      *@@=    *@@@@@@@@@@@@=:           :-%@@@@@@@@@\r\n");
    uart_puts("@@@+  -#@@@%-  =@@@@@@@@@-     #@@@@@@@@%   =%@@@=    *@@@@@@@@@@@@@#-         :*%@@@@@@@@@@\r\n");
    uart_puts("@@=:+%@@@@@@@%+--=*%@@@@@-    :@@@@@@#*=-=*@@@@@@=    #@@@@@@@@@@@@@@@*       =@@@@@@@@@@@@@\r\n");
    uart_puts("@%%@@@@@@@@@@@@@@@%##@@@@-    #@@@@%#%@@@@@@@@@@@=   +@@@@@@@@@@@@@@@@@*     -@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@-  =%@@@@@@@@@@@@@@@@@@@=  *@@@@@@@@@@@@@@@@@@@:    %@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@++%@@@@@@@@@@@@@@@@@@@@@*+%@@@@@@@@@@@@@@@@@@@@+   -@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%   +@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   %@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=  %@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@= :@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@* =@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@% =@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ #@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-%@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=@@@@@@@@@@@@@@@@@\r\n");
    uart_puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%@@@@@@@@@@@@@@@@@\r\n");
    uart_puts("============================================================================================\r\n");
}

void do_cmd_help()
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
}

void do_cmd_info()
{
    // print hw revision
    pt[0] = 7 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
        uart_puts("Hardware Revision\t: ");
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
}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 115200;
}

void do_cmd_reboot_cancel() {
    uart_puts("reboot_cancel\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0;
}

void do_cmd_ls()
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = (struct cpio_newc_header *)CPIO_DEFAULT_PLACE;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        //if this is not TRAILER!!! (last of file)
        if(header_ptr!=0) 
        {
            uart_puts(c_filepath);
            uart_puts("\r\n");
        }
        
    }
}

void do_cmd_cat(char* filepath)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = (struct cpio_newc_header *)CPIO_DEFAULT_PLACE;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if(strcmp(c_filepath, filepath)==0)
        {
            uart_puts_Readfile(c_filedata,c_filesize);
            break;
        }

        //if this is TRAILER!!! (last of file)
        if(header_ptr==0)
        {
            uart_puts("cat: ");
            uart_puts(filepath);
            uart_puts(": No such file or directory\n");
        }
    }
}

void do_cmd_malloc()
{
    //test malloc
    char* test1 = malloc(0x18);
    memcpy(test1,"test malloc1",sizeof("test malloc1"));
    uart_puts(test1);uart_puts("\n");

    char* test2 = malloc(0x20);
    memcpy(test2,"test malloc2",sizeof("test malloc2"));
    uart_puts(test2);uart_puts("\n");

    char* test3 = malloc(0x28);
    memcpy(test3,"test malloc3",sizeof("test malloc3"));
    uart_puts(test3);uart_puts("\n");
}

void do_cmd_dtb()
{
    //uart_puts(CPIO_DEFAULT_PLACE);
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}
