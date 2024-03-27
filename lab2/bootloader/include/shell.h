#ifndef _SHELL_H_
#define _SHELL_H_

#define CLI_MAX_CMD 4
#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128
#define DO_CMD_FUNC(name) int name(int argc, char *argv[])

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    // int (*func)(char *argv, int argc);
    DO_CMD_FUNC((*func));
    char help[MSG_MAX_LEN];
} CLI_CMDS;
void cli_cmd_init();
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

// void do_cmd_help();
// void do_cmd_loadimg();
// void do_cmd_reboot();
DO_CMD_FUNC(do_cmd_help);
DO_CMD_FUNC(do_cmd_loadimg);
DO_CMD_FUNC(do_cmd_reboot);
DO_CMD_FUNC(do_cmd_cancel_reboot);

#endif /* _SHELL_H_ */
