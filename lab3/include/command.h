#ifndef _COMMAND_H
#define _COMMAND_H 

void cmd_help();
void cmd_hello();
void cmd_ls();
void cmd_cat(const char *filename);
void cmd_malloc(int m_size);
void cmd_reboot();
void cmd_cancel_reboot();
void cmd_mailbox();
void cmd_exec(const char *filename);
void cmd_simple_timer(unsigned int timeout, char *seconds);
void cmd_timeout(unsigned int timeout, const char *message, const char *seconds);
void cmd_not_found();

#endif

