#ifndef _SHELL_H_
#define _SHELL_H_

#define CLI_MAX_CMD 6
#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128


typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
    int (*func)(void);
} CLI_CMDS;

int start_shell();

void shell_init();
void cli_flush_buffer(char *, int);
void cli_cmd_read(char *);
void cli_cmd_exec(char *);
void cli_print_banner();

int do_cmd_help();
int do_cmd_hello();
int do_cmd_info();
int do_cmd_reboot();
int do_cmd_cancel_reboot();
int do_cmd_loadimg();


#endif /* _SHELL_H_ */
