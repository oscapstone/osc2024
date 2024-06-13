#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "exception.h"
#include "sched.h"
#include "list.h"
#include "cpu_context.h"
#include "memory.h"


#define SIGHUP      1       /* Hangup (POSIX).  */
#define SIGINT      2       /* Interrupt (ANSI).  */
#define SIGQUIT     3       /* Quit (POSIX).  */
#define SIGILL      4       /* Illegal instruction (ANSI).  */
#define SIGTRAP     5       /* Trace trap (POSIX).  */
#define SIGABRT     6       /* Abort (ANSI).  */
#define SIGIOT      6       /* IOT trap (4.2 BSD).  */
#define SIGBUS      7       /* BUS error (4.2 BSD).  */
#define SIGFPE      8       /* Floating-point exception (ANSI).  */
#define SIGKILL     9       /* Kill, unblockable (POSIX).  */
#define SIGUSR1     10      /* User-defined signal 1 (POSIX).  */
#define SIGSEGV     11      /* Segmentation violation (ANSI).  */
#define SIGUSR2     12      /* User-defined signal 2 (POSIX).  */
#define SIGPIPE     13      /* Broken pipe (POSIX).  */
#define SIGALRM     14      /* Alarm clock (POSIX).  */
#define SIGTERM     15      /* Termination (ANSI).  */
#define SIGSTKFLT   16      /* Stack fault.  */
#define SIGCHLD     17      /* Child status has changed (POSIX).  */
#define SIGCLD      SIGCHLD /* Same as SIGCHLD (System V).  */
#define SIGCONT     18      /* Continue (POSIX).  */
#define SIGSTOP     19      /* Stop, unblockable (POSIX).  */
#define SIGTSTP     20      /* Keyboard stop (POSIX).  */
#define SIGTTIN     21      /* Background read from tty (POSIX).  */
#define SIGTTOU     22      /* Background write to tty (POSIX).  */
#define SIGURG      23      /* Urgent condition on socket (4.2 BSD).  */
#define SIGXCPU     24      /* CPU limit exceeded (4.2 BSD).  */
#define SIGXFSZ     25      /* File size limit exceeded (4.2 BSD).  */
#define SIGVTALRM   26      /* Virtual alarm clock (4.2 BSD).  */
#define SIGPROF     27      /* Profiling alarm clock (4.2 BSD).  */
#define SIGWINCH    28      /* Window size change (4.3 BSD, Sun).  */
#define SIGIO       29      /* I/O now possible (4.2 BSD).  */
#define SIGPOLL     SIGIO   /* Pollable event occurred (System V).  */
#define SIGPWR      30      /* Power failure restart (System V).  */
#define SIGSYS      31      /* Bad system call.  */
#define SIGUNUSED   31

// 向前宣告 cpu_context_t 和 thread_t
typedef struct thread_struct thread_t;

#define SIGNAL_COPY(dest_thread, src_thread)                                                         \
    do                                                                                               \
    {                                                                                                \
        dest_thread->signal = src_thread->signal;                                                    \
        DEBUG("Copy signal from %d to %d\n", src_thread->pid, dest_thread->pid);                     \
        DEBUG("handler_table[9]: 0x%x -> 0x%x\n", src_thread->signal.handler_table[SIGKILL],         \
              dest_thread->signal.handler_table[SIGKILL]);                                           \
        dest_thread->signal.pending_list = (signal_node_t *)kmalloc(sizeof(signal_node_t));          \
        DEBUG("kmalloc pending_list 0x%x\n", dest_thread->signal.pending_list);                      \
        INIT_LIST_HEAD((list_head_t *)dest_thread->signal.pending_list);                             \
        DEBUG("INIT_LIST_HEAD pending_list\n");                                                      \
        list_head_t *curr;                                                                           \
        list_for_each(curr, (list_head_t *)src_thread->signal.pending_list)                          \
        {                                                                                            \
            signal_node_t *new_node = kmalloc(sizeof(signal_node_t));                                \
            DEBUG("kmalloc new_node 0x%x\n", new_node);                                              \
            new_node->signal = ((signal_node_t *)curr)->signal;                                      \
            DEBUG("Copy signal 0x%x -> 0x%x\n", ((signal_node_t *)curr)->signal, new_node->signal);  \
            list_add_tail((list_head_t *)new_node, (list_head_t *)dest_thread->signal.pending_list); \
        }                                                                                            \
    } while (0)

void init_thread_signal(thread_t *thread);

void kernel_lock_signal(thread_t *thread);
void kernel_unlock_signal(thread_t *thread);
int8_t signal_is_lock();


void signal_send(int pid, int signal);
void signal_register_handler(int signal, void (*handler)());
void signal_default_handler();
void signal_killsig_handler();
int8_t has_pending_signal();
void run_pending_signal(trapframe_t *tpf);
void run_signal(int signal, uint64_t sp_el0);
void signal_handler_wrapper(char *dest);
void signal_return();


#endif /* _SIGNAL_H_ */