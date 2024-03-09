#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128

void shell_main();

void read_cmd();

void cmd_help();
void cmd_hello();
void cmd_reboot();

#endif