#ifndef RELOCATE_H
#define RELOCATE_H

extern char __start;
extern char __end;
extern char __bootloader_start;

void relocate(char *arg);

#endif