#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>

void* simple_malloc(int, int);

void reserve(unsigned long l, unsigned long r);
void frame_init();

void* frame_malloc(size_t size);
void* my_malloc(size_t size);

void frame_free(char*);
void my_free(char*);

void print_node_list();
void print_pool();

void manage_init ();

#endif
