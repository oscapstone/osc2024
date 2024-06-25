#ifndef	_SHELL_H
#define	_SHELL_H

#define MAX_CMD_NO 4
#define MAX_CMD_LEN 32
#define MAX_MSG_LEN 128

void cli_start_shell();
int  cli_strcmp(const char* p1, const char* p2);
void cli_read_cmd(char* buf);
void cli_exec_cmd(char* buf);
void cli_clear_cmd(char* buf, int length);
void cli_print_welcome_msg();
void cmd_help();
void cmd_hello();
void cmd_hwinfo();
void cmd_reboot();
void cmd_ls(char **argv, int argc);
void cmd_cat(char* filepath);
void cmd_exec_program(char **argv, int argc);
void cmd_malloc();
void cmd_dtb();
void cmd_currentEL();
void cmd_enable_timer();
void cmd_set_alert_2s(char**argvs, int argc);
void cmd_sleep(char** argvs, int argc);
void cmd_kmalloc();
int  cmd_mkdir(char **argv, int argc);
int  cmd_cd(char **argv, int argc);
int  cmd_write(char **argv, int argc);

#endif  /*_SHELL_H */
