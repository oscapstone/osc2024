#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
    void (*func)(void);
} CLI_CMDS;

int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

void do_cmd_help();
void do_cmd_hello();
void do_cmd_info();
void do_cmd_reboot();
void do_cmd_reboot_cancel();
void do_cmd_ls();
void do_cmd_cat();
void do_cmd_malloc();
void do_cmd_dtb();

#endif /* _SHELL_H_ */
