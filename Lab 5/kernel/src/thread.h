#ifndef __THREAD_H__
#define __THREAD_H__

#include "list.h"
#include "type.h"
#include "task.h"


void        thread_exit(int32_t status);
task_ptr    thread_create(uint64_t fn, uint64_t arg);
int32_t     thread_create_user(uint64_t fn, uint64_t program, uint32_t size);
int32_t     thread_replace_user(uint64_t fn, uint64_t program, uint32_t size);
int32_t     thread_create_shell(uint64_t fn);
int32_t     thread_fork();

void 		thread_demo();


#endif