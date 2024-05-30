#ifndef _MAILBOX_H
#define _MAILBOX_H

int mailbox_call(unsigned char ch, unsigned int *mailbox);
unsigned int get_board_revision();
void get_memory_info(unsigned int *base, unsigned int *size);

#endif
