#ifndef MAILBOX_H
#define MAILBOX_H

extern volatile unsigned int mbox[36];

int mbox_call(unsigned char ch);
int mbox_get_board_revision(void);
int mbox_get_arm_memory(void);
void print_board_revision(void);
void print_arm_memory(void);

#endif /* MAILBOX_H */
