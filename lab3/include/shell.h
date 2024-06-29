#pragma once

#define SHELL_BUF_SIZE 256

void welcome_msg();
void run_shell();
void read_user_input(char *buf);
void buffer_overflow_message();
int exec_command(const char *command);
