#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
} CLI_CMDS;

int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

void cmd_cat(char*);
void cmd_dtb();
void cmd_help();
void cmd_hello();
void cmd_info();
void cmd_malloc();
void cmd_ls(char*);
void cmd_reboot();

#endif /* _SHELL_H_ */
