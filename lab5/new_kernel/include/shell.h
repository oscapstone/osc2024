#ifndef _SHELL_H_
#define _SHELL_H_

#define CLI_MAX_CMD 11
#define CMD_MAX_LEN 32
#define CMD_MAX_LEN2 128
#define CMD_MAX_PARAM 10
#define MSG_MAX_LEN 128
#define USTACK_SIZE 0x10000

typedef struct CLI_CMDS
{
    char command[CMD_MAX_LEN];
    char help[MSG_MAX_LEN];
    int (*func)(int, char **);
} CLI_CMDS;

int shell_cmd_strcmp(const char* , const char*);
void shell_cmd_read(char*);
void shell_cmd_exe(char*);
void shell_cmd_clean(char*);
void shell();
void shell_banner();
void shell_help();
void shell_hello_world();
void shell_info();
void shell_reboot();
void shell_ls(int argc, char **argv);
void shell_cat(int argc, char **argv);
void shell_malloc();
void shell_dtb();
void shell_exec(int argc, char **argv);
void shell_setTimeout(int argc, char **argv);
void shell_timer();
void shell_2sTimer();
void shell_page_dump();
void shell_page_alloc();
void shell_cache_alloc();




#endif
