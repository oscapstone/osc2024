#ifndef STRING_H
#define STRING_H

unsigned int strlen(const char* str);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, unsigned int n);
unsigned int hexstr2int(char* hex_str);
char* strtok(char* str, const char* delim);
char* strcpy(char* dest, const char* src);

#endif /* STRING_H */
