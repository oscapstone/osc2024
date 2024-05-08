/**
 * Ref: http://courses.cms.caltech.edu/cs124/lectures-wi2016/CS124Lec15.pdf
*/
#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#define SIGKILL 9

#define NR_SIGNALS 10

// typedef void (*sighandler)(int);
typedef void (*sighandler_t)(void);

struct sighandler {
    sighandler_t action[10];
};

extern struct sighandler default_sighandler;


void sigreturn(void);



#endif // __SIGNAL_H__