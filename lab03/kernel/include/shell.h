#ifndef __SHELL_H__
#define __SHELL_H__

typedef struct cmd {
    char *name;
    void (*func)(int argc, char *argv[]);
    char help_msg[100];
} cmd;

void readcmd(char [256]);
void shell();
void async_shell();

#endif