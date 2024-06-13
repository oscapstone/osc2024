#ifndef	_BOOT_H
#define	_BOOT_H

extern void delay ( unsigned long);
extern void put32 ( volatile unsigned long, unsigned int );
extern unsigned int get32 ( volatile unsigned long );

#endif  /*_BOOT_H */
