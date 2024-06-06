#include "mini_uart.h"
#include "mail.h"

int mailbox_call(unsigned int *mailbox, unsigned char ch) {
	unsigned int r = (unsigned int)(((unsigned long)mailbox) & (~0xF)) | (ch & 0xF);
	while(*MAILBOX_STATUS & MAILBOX_FULL);
	*MAILBOX_WRITE = r;
    while(1) {
    	while(*MAILBOX_STATUS & MAILBOX_EMPTY);
		// uart_send_string("Trying to get the result from mailbox...\r\n");
        if(r == *MAILBOX_READ)
            return mailbox[1] == REQUEST_SUCCEED;
    }
    return 0;
}

unsigned int get_board_revision(){
	unsigned int __attribute__((aligned(16))) mailbox[7];
	mailbox[0] = 7 * 4; // buffer size in bytes
	mailbox[1] = REQUEST_CODE;
	// tags begin
	mailbox[2] = GET_BOARD_REVISION; // tag identifier
	mailbox[3] = 4; // maximum of request and response value buffer's length.
	mailbox[4] = TAG_REQUEST_CODE;
	mailbox[5] = 0; // value buffer
	// tags end
	mailbox[6] = END_TAG;
	
	if (mailbox_call(mailbox, 8) ) {
		return mailbox[5];	
	}
	else {
		return 0;
	}
}

int get_arm_memory(unsigned int* arr) {
	unsigned int __attribute__((aligned(16))) mailbox[8];
	mailbox[0] = 8 * 4; // buffer size in bytes
	mailbox[1] = REQUEST_CODE;
	// tags begin
	mailbox[2] = GET_ARM_MEMORY; // tag identifier
	mailbox[3] = 8; // maximum of request and response value buffer's length.
	mailbox[4] = TAG_REQUEST_CODE;
	mailbox[5] = 0; // value buffer
	mailbox[6] = 0; // value buffer
	// tags end
	mailbox[7] = END_TAG;
	
	if (mailbox_call(mailbox, 8)) {
		arr[0] = mailbox[5]; arr[1] = mailbox[6];
		return 0;
	}
	else {
		return -1;
	}
}
