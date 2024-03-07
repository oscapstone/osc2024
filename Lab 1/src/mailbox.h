#ifndef MAILBOX_H
#define MAILBOX_H


/* a properly aligned buffer */
extern volatile unsigned int mbox[36];

int mailbox_call(unsigned char ch);

void mailbox_get_board_revision();
void mailbox_get_arm_memory();
void mailbox_get_vc_info();

#endif