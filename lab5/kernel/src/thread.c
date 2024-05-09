#include "thread.h"
#include "alloc.h"

void thread_init() {
}

void kill_zombies() {

}

void idle() {
	while (1) {
		kill_zombies();
		schedule();
	}
}

void schedule() {
		
}
