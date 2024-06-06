#ifndef SHELL_H
#define SHELL_H

void shell_init();
void shell_input(char *cmd);
void shell_controller(char *cmd);

void do_shell_user(void);
void do_shell(void);
void do_user_image(void);

#endif