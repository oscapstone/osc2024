#ifndef MYSHELL_H
#define MYSHELL_H

#include "../include/my_stddef.h"

#define MAX_BUFFER_LEN 64
#define NEW_LINE 1001
#define LINE_FEED 10
#define CRRIAGE_RETURN 13
#define BACKSPACE 8


void shell_start();
void command_controller(char c, char buffer[], int *counter, char **argv);
void execute_command(int argc, char **argv);
int parse_arguments(char *buffer, char **argv);

#endif
