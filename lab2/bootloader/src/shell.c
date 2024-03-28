#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"

extern char* _dtb;
extern char _start[];

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="loadimg", .help="load image via uart1", .func = do_cmd_loadimg},
    {.command="help", .help="print all available commands", .func = do_cmd_help},
    {.command="reboot", .help="reboot the device", .func = do_cmd_reboot},
    {.command="c", .help="cancel reboot", .func = do_cmd_reboot_cancel}
};
    
void shell_init()
{
    cmd_list[0].func = do_cmd_loadimg;
    cmd_list[1].func = do_cmd_help;
    cmd_list[2].func = do_cmd_reboot;
    cmd_list[3].func = do_cmd_reboot_cancel;
}
int cli_cmd_strcmp(const char* p1, const char* p2)
{
    const unsigned char *s1 = (const unsigned char*) p1;
    const unsigned char *s2 = (const unsigned char*) p2;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if ( c1 == '\0' ) return c1 - c2;
    } while ( c1 == c2 );
    return c1 - c2;
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

    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
    	
        if (cli_cmd_strcmp(buffer, cmd_list[i].command) == 0)
        {
            cmd_list[i].func();
            return;            
        }
    }
    if (*buffer)
    {
        uart_puts(buffer);
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

void do_cmd_loadimg()
{
    char* bak_dtb = _dtb;
    char c;
    unsigned long long kernel_size = 0;
    char* kernel_start = (char*) (&_start);
    uart_puts("Please upload the image file.\r\n");
    for (int i=0; i<8; i++)
    {
        c = uart_recv_loading();
        kernel_size += c<<(i*8);
    }
    for (int i=0; i<kernel_size; i++)
    {
        c = uart_recv_loading();
        kernel_start[i] = c;
    }
    uart_puts("Image file downloaded successfully.\r\n");
    uart_puts("Point to new kernel ...\r\n");

    ((void (*)(char*))kernel_start)(bak_dtb);
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

