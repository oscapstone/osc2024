#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 0x100
#define MSG_MAX_LEN 0x100
#define DO_CMD_FUNC(name) int name(char *argv[], int argc)

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    int (*func)(char* argv[], int argc);
    char help[MSG_MAX_LEN];
} CLI_CMDS;

void cli_cmd_init();
void cli_cmd();
int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

DO_CMD_FUNC(do_cmd_help);
DO_CMD_FUNC(do_cmd_s_allocator);
DO_CMD_FUNC(do_cmd_memory_tester);
#endif /* _SHELL_H_ */
