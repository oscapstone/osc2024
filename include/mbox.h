#ifndef _MBOX_H
#define _MBOX_H

extern volatile unsigned int mbox[36]; /* a properly aligned buffer */

void get_arm_base_memory_sz();
void get_board_serial();
void get_board_revision();
int mbox_call(unsigned char ch);

#endif