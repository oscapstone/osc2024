#ifndef __SHELL_H_
#define __SHELL_H_

#define MAX_BUFFER_SIZE 256
#define MAX_PARAM_NUMBER 10

void shell(void);

void read_command(char*);

void parsing(char*);

void exec(char*, char**, int);

#endif
