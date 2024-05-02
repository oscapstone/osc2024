#ifndef _SHELL_H_
#define _SHELL_H_

#define CLI_MAX_CMD 12
#define CMD_MAX_LEN 32
#define CMD_MAX_PARAM 10
#define MSG_MAX_LEN 128

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
    int (*func)(int, char **);
} CLI_CMDS;

int _parse_args(char *buffer, int *argc, char **argv);

int start_shell();

void cli_flush_buffer(char *, int);
void cli_cmd_read(char *);
void cli_cmd_exec(char *);
void cli_print_banner();

int do_cmd_help(int argc, char **argv);
int do_cmd_hello(int argc, char **argv);
int do_cmd_info(int argc, char **argv);
int do_cmd_reboot(int argc, char **argv);
int do_cmd_ls(int argc, char **argv);
int do_cmd_cat(int argc, char **argv);
int do_cmd_malloc(int argc, char **argv);
int do_cmd_dtb(int argc, char **argv);
int do_cmd_exec(int argc, char **argv);
int do_cmd_setTimeout(int argc, char **argv);
int do_cmd_set2sAlert(int argc, char **argv);
int do_cmd_mtest(int argc, char **argv);
#endif /* _SHELL_H_ */
