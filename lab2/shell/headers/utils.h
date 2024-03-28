#ifndef __UTILS_H
#define __UTILS_H

#define MAX_LEN 512

// string functions
int strcmp(const char*, const char*);
int strlen(const char *);

// interger functions
void bin2hex(unsigned int, char*);
void bin2dec(unsigned int, char*);
unsigned int atoi(char*, unsigned int);

// address function
void align(unsigned int*, unsigned int);

#endif