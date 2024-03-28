#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128
#define ARG_MAX_NUM 10
#define ARG_MAX_LEN 128

void shell_main();

void read_cmd();
void read_arg();

void cmd_help();
void cmd_hello();
void cmd_reboot();
void cmd_cat();
void cmd_ls();
void cmd_dtb();

#endif