extern volatile unsigned int mbox[36];

int mailbox_call(unsigned char ch);
void get_board_revision();
void get_arm_memory();