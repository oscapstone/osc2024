#ifndef _SHELL_H
#define _SHELL_H

void shell();
int read_cmd(char* str);
void parse_cmd(char* str);
int strcmp(char *str1, char *str2);

#endif