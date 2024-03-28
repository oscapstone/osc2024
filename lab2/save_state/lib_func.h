typedef unsigned int size_t;

int strcmp(char *s1, char *s2);
void memset(void *ptr, int value, size_t num);
int memcmp(void *s1, void *s2, int n);
int hex2bin(char *s, int n);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);
char *strtok(char *str, const char *delim);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);