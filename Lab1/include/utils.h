#ifndef _UTILS_H_
#define _UTILS_H_

// Power
#define PM_PASSWORD 0x5A000000
#define PM_RSTC     0x3F10001C
#define PM_WDOG     0x3F100024

/* return 1 if a == b */
int strcmp(const char *a, const char *b);

#endif