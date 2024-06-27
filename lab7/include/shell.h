#pragma once

#define SHELL_BUF_SIZE 1024

void run_shell();
void read_user_input(char *buf);
int exec_command(const char *command);
