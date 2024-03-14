#ifndef STRING_H
#define STRING_H
#include "type.h"

int  strcmp     ( char * s1, char * s2 );
void strset     ( char * s1, int c, int size );
int  strlen     ( char * s );
void itohex_str ( uint64_t d, int size, char * s );

#endif