#ifndef STDDEF_H
#define STDDEF_H

#define NULL 0
#define size_t unsigned long

/* Offset of member MEMBER in a struct of type TYPE. */
#undef offsetof		/* in case a system header has defined it. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

#endif
