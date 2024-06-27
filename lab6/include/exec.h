#ifndef _EXEC_H
#define _EXEC_H

void exec_user_prog(void *entry, char *user_sp, char *kernel_sp);
void enter_el0_run_user_prog(void *entry, char *user_sp);

#endif /* _EXEC_H */