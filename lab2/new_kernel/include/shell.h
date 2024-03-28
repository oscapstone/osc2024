#ifndef _SHELL_H_
#define _SHELL_H_

#define CMD_MAX_LEN 32
#define CMD_MAX_LEN2 128


int shell_cmd_strcmp(const char* , const char*);
void shell_cmd_read(char*);
void shell_cmd_exe(char*);
void shell_cmd_clean(char*);
void shell();
void shell_banner();




#endif
