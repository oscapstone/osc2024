#ifndef SIGNAL_H
#define SIGNAL_H

void handle_signal(void*);
void sigkill();
void signal_handler_wrapper(void*);

#endif
