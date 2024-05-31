#pragma once


// copy from /include/bits/mman.h
#define PROT_NONE           0
#define PROT_READ           1
#define PROT_WRITE          2
#define PROT_EXEC           4

#define MAP_ANONYMOUS       0x20                /* Don't use a file.  */
#define MAP_POPULATE        0x08000                /* Populate (prefault) pagetables.  */