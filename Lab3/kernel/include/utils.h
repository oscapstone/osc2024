#ifndef UTILS_H
#define UTILS_H

extern void put32(unsigned long addr, unsigned int val);
extern unsigned int get32(unsigned long addr);
extern void delay(unsigned long cl);
extern unsigned int get_el(void);

#endif /* UTILS_H */
