
#include "lib/mbox_call.h"
#include "lib/uartwrite.h"
#include "lib/printf.h"

volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

int main() {

    // getting arm mem info
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = MBOX_REQUEST;    
    // tags begin
    mailbox[2] = MBOX_TAG_GETARMMEM; // tag identifier
    mailbox[3] = 8;          // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP, mailbox)) {
        printf("Failed to call mailbox system call.\n");
        return -1;
    }

    printf("memory size: 0x%x.\n", mailbox[6]);

    return 0;
}
