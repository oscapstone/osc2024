#ifndef __MAILBOX_H
#define __MAILBOX_H

extern volatile unsigned int mailbox[36];

int mailbox_call(unsigned char ch);
void get_board_revision();
void get_arm_memory();

#endif