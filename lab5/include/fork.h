#ifndef FORK_H
#define FORK_H

int copy_process(unsigned long fn, unsigned long arg);
void ret_from_fork(void);

#endif