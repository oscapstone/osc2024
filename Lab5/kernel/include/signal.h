#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "thread.h"

typedef struct thread thread_t;
typedef struct trap_frame trap_frame_t;

void check_signal(trap_frame_t*);
void handle_signal(trap_frame_t*, int);
void signal_handler_run();
void user_signal_return();

void default_signal_handler();

#endif