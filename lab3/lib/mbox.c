#include "mbox.h"

int mbox_call(mail_t* mbox, uint8_t ch) {
    uint32_t addr_channel;

    // 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    addr_channel = (((uint32_t)(uint64_t)mbox) & 0xFFFFFFF0) | (ch & 0xF);

    // 2. Read the status register until the empty flag is not set, then break while
    while (mmio_read(MBOX_STATUS) & MBOX_FULL) asm volatile("nop");

    // 3. If not, then you can write to Mailbox 1 Read/Write register.
    mmio_write(MBOX_WRITE, addr_channel);

    // 4. Check if Mailbox 0 status register’s empty flag is set.
    // 5. Read from Mailbox 0 Read/Write register.
    do {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) asm volatile("nop");
    } while (mmio_read(MBOX_READ) != addr_channel);

    
    // 6. And check if the value is the same as you wrote in step 1.
    // 0x80000000: request successfu
    return mbox->header.code == MBOX_RESPONSE;
}

void get_board_revision(uint32_t* board_revision) {
    // Create a mailbox structure and initialize its fields
    mail_t mbox = {
        .header.packet_size = MAIL_PACKET_SIZE,   // Set the packet size in the header
        .header.code = REQUEST_CODE,              // Set the code in the header to indicate a request
        .body.id = GET_BOARD_REVISION,            // Set the ID of the body to get board revision
        .body.buf_size = MAIL_BUF_SIZE,           // Set the size of the buffer in the body
        .body.code = TAG_REQUEST_CODE,            // Set the code in the body to indicate a tag request
        .body.end = END_TAG,                      // Set the end tag in the body
    };

    // Initialize the buffer in the body to 0
    for (uint32_t i = 0; i < MAIL_BODY_BUF_LEN; i++) {
        mbox.body.buf[i] = 0;
    }

    // Call the mailbox function to send the request and receive the response
    // MBOX_CH_PROP means chennel 8
    mbox_call(&mbox, MBOX_CH_PROP);

    // Extract the board revision information from the response buffer and assign it to board_revision
    *board_revision = mbox.body.buf[0];
}


// Get ARM memory Response:Length: 8
void get_memory_info(uint32_t* mem_base, uint32_t* mem_size) {
    mail_t mbox = {
        .header.packet_size = MAIL_PACKET_SIZE,
        .header.code = REQUEST_CODE,
        .body.id = GET_ARM_MEMORY,
        .body.buf_size = MAIL_BUF_SIZE,
        .body.code = TAG_REQUEST_CODE,
        .body.end = END_TAG,
    };

    for (uint32_t i = 0; i < MAIL_BODY_BUF_LEN; i++) mbox.body.buf[i] = 0;

    if (mbox_call(&mbox, MBOX_CH_PROP)) {
        *mem_base = mbox.body.buf[0]; // u32: base address in bytes
        *mem_size = mbox.body.buf[1]; // u32: size in bytes
        
    } else {
        *mem_size = 0xFFFFFFFF;
        *mem_base = 0xFFFFFFFF;
    }
}


// Get board serial : Response:Length: 8 u64: board serial
void get_board_serial(uint32_t* msb, uint32_t* lsb) {
    mail_t mbox = {
        .header.packet_size = MAIL_PACKET_SIZE,
        .header.code = REQUEST_CODE,
        .body.id = GET_BOARD_SERIAL,
        .body.buf_size = MAIL_BUF_SIZE,
        .body.code = TAG_REQUEST_CODE,
        .body.end = END_TAG,
    };

    for (uint32_t i = 0; i < MAIL_BODY_BUF_LEN; i++) mbox.body.buf[i] = 0;

    if (mbox_call(&mbox, MBOX_CH_PROP)) {
        *msb = mbox.body.buf[1];
        *lsb = mbox.body.buf[0];
    } else {
        *msb = 0xFFFFFFFF;
        *lsb = 0xFFFFFFFF;
    }
}


