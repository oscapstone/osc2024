#ifndef _MEM_H
#define _MEM_H

#include "types.h"

#define MAX_ORDER 16

// #define MEM_START 0x10000000  // 256MB memory space & 4KB frame
// #define MEM_END 0x20000000    // -> # of frames = 2^16

// advanced part II
#define MEM_START 0x00
#define MEM_END 0x3C000000
#define SPIN_TABLE_START ((void*)0x0000)
#define SPIN_TABLE_END ((void*)0x1000)

#define FREE 0
#define ALLOCATED 1
#define BUDDY -1

#define FRAME_SIZE (1 << 12)
#define FRAME_CNT ((MEM_END - MEM_START) / FRAME_SIZE)
#define MAX_CHUNK_SIZE 2048
#define CHUNK_SIZE_TYPES 8

typedef struct frame_node {
  struct frame_node *prev, *next;
  void* addr;
} frame_node;

typedef struct frame_entry {
  int16_t order;
  uint16_t status;
  struct frame_node* node;
} frame_entry;

// for fine-grained memory allocation
typedef struct chunk_node {
  struct chunk_node* next;
  void* addr;
} chunk_node;

typedef struct chunk_entry {
  uint16_t is_mem_pool;
  uint16_t size;
  uint16_t free_chunk_cnt;
} chunk_entry;

void init_mem();
void* malloc(uint32_t size);
void free(void* addr);

#endif