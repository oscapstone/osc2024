#ifndef __USER_DEMO_H__
#define __USER_DEMO_H__

#define delay(x) for(int i=0; i<x; i++) asm volatile("nop");

void multiple_thread_test(int argc, char *argv[]);
void user_fork_test(int argc, char *argv[]);
void user_open_test(int argc, char *argv[]);
void user_open_test_initramfs(int argc, char *argv[]);
void user_read_test(int argc, char *argv[]);
void user_write_test(int argc, char *argv[]);
void user_write_test_initramfs(int argc, char *argv[]);

#endif