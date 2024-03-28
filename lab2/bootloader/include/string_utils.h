#ifndef	STRING_UTILS_H
#define	STRING_UTILS_H

int my_strcmp(char *s1, char *s2);
void newline_to_nullbyte(char *buffer);
unsigned char hexchar2val(char ch);
unsigned int hexstr2val(char *str, unsigned int size);
void mem_align(void *addr, unsigned int number);

#endif  