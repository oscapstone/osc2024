#ifndef SHELL_H
#define SHELL_H

#define MAX_CMD_LEN 256

void show_banner();
void shell_run();
void format_command(const char *command, const char *description);
void do_cmd_help();
void do_cmd_info();
void do_cmd_clear();
void do_cmd_ls();
void do_cmd_cat(const char *args);
void do_cmd_malloc();
void do_cmd_dtb();

#endif