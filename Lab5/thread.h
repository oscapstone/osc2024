//set the start running address of thread to lr
struct registers {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long lr;
	unsigned long sp;
};

struct thread{
    struct registers regs;
    int pid;
    int state; //run queue, running, wait queue, zombie... (run/wait not implemented yet) 
    int parent; //-1: no parent
    int priority;
    void (*funct)(void); //task
    unsigned long sp_el1;
	unsigned long sp_el0;
};

typedef struct thread thread;
int create_thread(void * function, int prioirty);
void schedule();
int get_pid();
void test_exec();
void idle();
void thread_init();
void thread_execute();
void thread_exit();