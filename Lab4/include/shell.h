#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 0x100
#define MSG_MAX_LEN 0x100

typedef struct CMDS
{
    char cmd[CMD_MAX_LEN];
    char msg[MSG_MAX_LEN];
} CMDS;


void cmd_clear(char* buffer, int len);
void cmd_read(char* buffer);
void cmd_exec(char *buffer);
void cmd_hello();
void cmd_info();
void cmd_help();
void cmd_dtb();
void cmd_ls();
void cmd_cat(char *file);
void cmd_2stimeout();
void cmd_timeout(char *sec, char *message);
void cmd_reboot();
void cmd_memory();
void cmd_error(char *cmd);

#endif /* _SHELL_H_ */