#ifndef _DEF_STRING
#define _DEF_STRING

int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strlen_new(const char* s);
int hex_atoi(const char *, int);
int getdelim(char **, int, int);
int getline(char **, int);
void *memcpy(void *, const void *, int);

#endif
