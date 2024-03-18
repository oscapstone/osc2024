#ifndef STRING_H
#define STRING_H


int str_cmp(const char *s1, const char *s2);
int str_eql(const char *s1, const char *s2);

void uint_to_hex(unsigned int n, char *buffer);
void uint_to_ascii(unsigned int n, char *buffer);

#endif