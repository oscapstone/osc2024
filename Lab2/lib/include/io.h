#ifndef _IO_H
#define _IO_H

#include "type.h"
#include "uart.h"

void print_char(char);
void print_str(char*);
void print_hex(uint32_t);
void print_nchar(char*, int);
void print_newline();
void read_input(char*);

#endif