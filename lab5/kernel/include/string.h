#ifndef STRING_H
#define STRING_H



unsigned long long strlen(const char *str);
char *strcpy(char *dest, const char *src);
void _strcpy(char *dest, char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
char* strtok(char* str, const char* delimiters);

int isalpha(char c);
int isdigit(int c);
int toupper(int c);
int isxdigit(int c);
long long atoi(char *str);
unsigned int parse_hex_str(char *arr, int size);



#endif