#ifndef _DEF_SHELL
#define _DEF_SHELL

#define MAX_GETLINE_LEN 0x20

void shell_start(void);
void do_cmd(const char*);

typedef void (*cmt_t)();
void cmd_help(void);
void cmd_hello(void);
void cmd_reboot(void);
void cmd_default(void);

#endif
