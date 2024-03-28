#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define MSG_MAX_LEN 128
#define ARG_MAX_NUM 10
#define ARG_MAX_LEN 128

#define KERNEL_ADDR 0x80000
#define TEMP_ADDR   0x60000

void shell_main();

void read_cmd();

void cmd_help();
void cmd_hello();
void cmd_reboot();
void cmd_loadimg();
void cmd_loadimg2();
void cmd_test();

#endif