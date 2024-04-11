#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128
#define DO_CMD_FUNC(name) int name(int argc, char *argv[])


typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    DO_CMD_FUNC((*func));
    char help[MSG_MAX_LEN];
} CLI_CMDS;

int  cli_cmd_strcmp(const char*, const char*);
void cli_cmd_clear(char*, int);
void cli_cmd_read(char*);
void cli_cmd_exec(char*);
void cli_print_banner();

DO_CMD_FUNC(do_cmd_cat   );
DO_CMD_FUNC(do_cmd_dtb   );
DO_CMD_FUNC(do_cmd_help  );
DO_CMD_FUNC(do_cmd_hello );
DO_CMD_FUNC(do_cmd_info  );
DO_CMD_FUNC(do_cmd_malloc);
DO_CMD_FUNC(do_cmd_ls    );
DO_CMD_FUNC(do_cmd_reboot);
DO_CMD_FUNC(do_cmd_cancel_reboot);

#endif /* _SHELL_H_ */
