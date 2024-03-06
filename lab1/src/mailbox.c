#include "mailbox.h"

volatile unsigned int __attribute__((aligned(16))) mailbox[36];

// return 0 if fail
int mailbox_call(unsigned char channel){
    // 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    unsigned int mes_addr = (unsigned long)&mailbox;
    unsigned int write_val = (mes_addr & ~0xF) | (channel & 0xF);

    // 2. Check if Mailbox 0 status register’s full flag is set.
    while(get32(MAILBOX_STATUS) & MAILBOX_FULL)
        asm volatile("nop");

    // 3. If not, write to Mailbox 1 Read/Write register.
    put32(MAILBOX_WRITE, write_val);

    while(1){
        // 4. Check if Mailbox 0 status register’s empty flag is set.
        while(get32(MAILBOX_STATUS) & MAILBOX_EMPTY)
            asm volatile("nop");

        // 5. If not, read from Mailbox 0 Read/Write register.
        // 6. Check if the value is the same as you wrote in step 1.
        if(get32(MAILBOX_READ) == write_val)
            // mailbox[1] will be written to val if success
            return mailbox[1] == MAILBOX_RESPONSE;
    }
    return 0;
}

int get_arm_memory(unsigned int *base_addr, unsigned int *mem_size){
    mailbox[0] = 7 * 4;
    mailbox[1] = MAILBOX_REQUEST;
    mailbox[2] = MAILBOX_TAG_GETARMMEM;
    mailbox[3] = 8;
    mailbox[4] = 0;
    mailbox[5] = 0;
    mailbox[6] = 0;
    mailbox[7] = MAILBOX_TAG_END;
    if(!mailbox_call(MAILBOX_CH_PROP))
        return -1;
    *base_addr = mailbox[5];
    *mem_size = mailbox[6];
    return 0;
}

int get_board_serial(unsigned long long *board_serial){
    mailbox[0] = 8 * 4;                 // message len in bytes
    mailbox[1] = MAILBOX_REQUEST;       // tell this is request message
    mailbox[2] = MAILBOX_TAG_GETSERIAL; // tag identifier,
    mailbox[3] = 8;                     // output buffer size in bytes
    mailbox[4] = MAILBOX_REQUEST;       // tag request
    mailbox[5] = 0;                     // output value buffer
    mailbox[6] = 0;                     // output value buffer
    mailbox[7] = MAILBOX_TAG_END;       // describe end of tag
    if(!mailbox_call(MAILBOX_CH_PROP))
        return -1;
    *board_serial = (((unsigned long long) mailbox[5]) << 32) | mailbox[6];
    return 0;
}

int get_board_revision(unsigned int *board_revision){
    mailbox[0] = 7*4;
    mailbox[1] = MAILBOX_REQUEST;
    mailbox[2] = MAILBOX_TAG_GETREVISION;
    mailbox[3] = 4;
    mailbox[4] = 0;
    mailbox[5] = 0; // value buffer
    mailbox[6] = MAILBOX_TAG_END;
    if(!mailbox_call(MAILBOX_CH_PROP))
        return -1;
    *board_revision = mailbox[5];
    return 0;
}




void print_board_info() {

    unsigned long long board_serial;
    get_board_serial(&board_serial);
    uart_send_string("board serial: ");
    uart_send_hex64(&board_serial);
    uart_send('\n');

    unsigned int board_revision;
    get_board_revision(&board_revision);
    uart_send_string("board revision: ");
    uart_send_hex(&board_revision);
    uart_send('\n');

    unsigned int base_addr = 0;
    unsigned int mem_size = 0;
    get_arm_memory(&base_addr, &mem_size);
    uart_send_string("arm memory: ");
    uart_send_hex(&base_addr);
    uart_send(' ');
    uart_send_hex(&mem_size);
    uart_send('\n');
}
