#ifndef FORK_H
#define FORK_H

struct task_struct* copy_process(void* fn, void* arg);

#endif /* FORK_H */
