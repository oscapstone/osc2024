#ifndef __M_STRING_H
#define __M_STRING_H



char* m_strtok(char *str, char delim, char **next);

// return 0 if identitical
int m_strcmp(const char * cs, const char * ct);
int m_strlen(const char *s);
int m_atoi(const char *s);
int m_htoi(const char *s);
char* m_itoa(int value, char *s);
// char* ftoa(float value, char *s);
unsigned int m_vsprintf(char *dst, char *fmt, __builtin_va_list args);
unsigned int m_sprintf(char *dst, char *fmt, ...);


#endif