#ifndef _SHELL_H_
#define _SHELL_H_

#define CLI_MAX_CMD 4
#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
    void (*func)(void);
} CLI_CMDS;

void shell_init();
int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

void do_cmd_loadimg();
void do_cmd_help();
void do_cmd_reboot();
void do_cmd_reboot_cancel();

#endif /* _SHELL_H_ */
