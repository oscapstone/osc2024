#include "../kernel/bsp/mailbox.h"

void mailbox_call(unsigned int *mailbox) {
  // Write the data (shifted into the upper 28 bits) combined with
  // the channel (in the lower four bits) to the write register.
  unsigned int r = (((unsigned long)mailbox) & ~0xf) | 8; // mail_ch_prop
  // & ~0xf => only "and" upper 28 bit can be saved
  // |8 => if upper 28 is 1 => save  and ensure last 4 bit is 1

  // Check if Mailbox 0 status register’s full flag is set.
  while (*MAILBOX_STATUS & MAILBOX_FULL) {
    asm volatile("nop");
  };

  // If not, then you can write to Mailbox 1 Read/Write register.
  *MAILBOX_WRITE = r;

  while (1) {
    // Check if Mailbox 0 status register’s empty flag is set.
    while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
      asm volatile("nop");
    };

    // If not, then you can read from Mailbox 0 Read/Write register.
    // Check if the value is the same as you wrote in step 1.
    if (r == *MAILBOX_READ)
      return;
  }
}

unsigned int get_board_revision() {
  unsigned int mailbox[7];
  mailbox[0] = 7 * 4; // buffer size in bytes
  mailbox[1] = REQUEST_CODE;
  // tags begin
  mailbox[2] = GET_BOARD_REVISION; // tag identifier
  mailbox[3] = 4; // maximum of request and response value buffer's length.
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0; // value buffer
  // tags end
  mailbox[6] = END_TAG;

  mailbox_call(mailbox); // message passing procedure call, you should implement
                         // it following the 6 steps provided above.
  return mailbox[5];
}

void get_memory_info(unsigned int *base_addr, unsigned int *size) {
  unsigned int mailbox[8];
  mailbox[0] = 8 * 4; // buffer size in bytes
  mailbox[1] = REQUEST_CODE;
  // tags begin
  mailbox[2] = GET_ARM_MEMORY; // tag identifier
  mailbox[3] = 8; // maximum of request and response value buffer's length.
  mailbox[4] = TAG_REQUEST_CODE; // tag code
  mailbox[5] = 0;                // base address
  mailbox[6] = 0;                // size in bytes
  mailbox[7] = END_TAG;          // end tag

  mailbox_call(mailbox); // message passing procedure call, you should implement
                         // it following the 6 steps provided above.

  // ARM memory base address
  *base_addr = mailbox[5];
  // ARM memory size
  *size = mailbox[6];
}
