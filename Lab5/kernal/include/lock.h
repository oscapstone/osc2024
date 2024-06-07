#ifndef LOCK_H
#define LOCK_H



// disable all interrupt to protect critical section
void lock(void);
// enable all interrupt to release critical section
void unlock(void);

#endif