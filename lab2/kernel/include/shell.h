#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    int (*func)(char* argv, int argc);
    char help[MSG_MAX_LEN];
} CLI_CMDS;

int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

int do_cmd_cat     (char *argv, int argc);
int do_cmd_dtb     (char *argv, int argc);
int do_cmd_help    (char *argv, int argc);
int do_cmd_hello   (char *argv, int argc);
int do_cmd_info    (char *argv, int argc);
int do_cmd_malloc  (char *argv, int argc);
int do_cmd_ls      (char *argv, int argc);
int do_cmd_reboot  (char *argv, int argc);
int do_cmd_cancel_reboot(char *argv, int argc);

#endif /* _SHELL_H_ */
