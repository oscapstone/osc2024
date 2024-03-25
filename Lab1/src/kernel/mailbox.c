#include "kernel/mailbox.h"

// The buffer itself is 16-byte aligned as only the upper 28 bits of the address can be passed via the mailbox.
volatile unsigned int __attribute__((aligned(16))) mailbox[32];

int mailbox_call(){
    // The mailbox interface has 28 bits (MSB) available for the value(message address) and 4 bits (LSB) for the channel
    // First take the 64bits address of mailbox(is probably just to ensure it can fits), then take the lower 32bits
    // clear LSB 4 bits(~0xF=1111 1111 1111 1111 1111 1111 1111 0000) and fill with channel number.
    unsigned int mbox_ptr = ((unsigned int)((unsigned long)&mailbox) & ~0xF) | (MAILBOX_CH_PROP & 0xF);

    // Wait until the mailbox is not full
    while((mmio_read((long)MAILBOX_STATUS) & MAILBOX_FULL)){
        asm volatile("nop");
    }
    // write our address containing message to mailbox address
    mmio_write((long)MAILBOX_WRITE, mbox_ptr);
    // Wait for response
    while(1){
        // until the mailbox is not empty
        while(mmio_read((long)MAILBOX_STATUS) & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        // if it's the response corresponded to our request
        if(mbox_ptr == mmio_read((long)MAILBOX_READ)){
            // if the response is successed
            return mailbox[1] == REQUEST_SUCCEED;
        }
    }

    return 0;
}

// regarding the message format:https://github.com/bztsrc/raspi3-tutorial/tree/master/04_mailboxes
void get_board_revision(){

    mailbox[0] = 7 * 4;               // buffer size in bytes (size of the message in bytes)
    mailbox[1] = REQUEST_CODE;        // MBOX_REQUEST magic value, indicates request message
    // tags begin
    mailbox[2] = GET_BOARD_REVISION;  // tag identifier
    mailbox[3] = 4;                   // maximum of request and response value buffer's length.(value buffer size in bytes)
    mailbox[4] = TAG_REQUEST_CODE;    // must be zero
    mailbox[5] = 0;                   // (optional) value buffer
    // tags end
    mailbox[6] = END_TAG;             // indicates no more tags

    //mailbox_call();            // message passing procedure call, you should implement it following the 6 steps provided above.

    //printf("0x%x\n", mailbox[5]);     // it should be 0xa020d3 for rpi3 b+
    if(mailbox_call()){
        uart_puts("My board revision: ");
        uart_b2x(mailbox[5]);           // 0x00A02082 for QEMU?
        uart_puts("\r\n");
    }
    else{
        uart_puts("Mailbox response error\n");
    }
}

void get_arm_mem(){
    mailbox[0] = 8 * 4;               // buffer size in bytes (size of the message in bytes) (response length is 8)
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEMORY;  // tag identifier
    mailbox[3] = 8;                   // maximum of request and response value buffer's length.(value buffer size in bytes)
    mailbox[4] = TAG_REQUEST_CODE;    // must be zero
    mailbox[5] = 0;                   // (optional) value buffer
    mailbox[6] = 0;                   // (optional) value buffer
    // tags end
    mailbox[7] = END_TAG;             // indicates no more tags

    if(mailbox_call()){
        uart_puts("My ARM memory base address: ");
        uart_b2x(mailbox[5]);           // 0x00A02082 for QEMU?
        uart_puts("\r\n");
        uart_puts("My ARM memory size: ");
        uart_b2x(mailbox[6]);           // 0x00A02082 for QEMU?
        uart_puts("\r\n");
    }
    else{
        uart_puts("Mailbox response error\n");
    }
}