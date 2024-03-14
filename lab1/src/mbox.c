#include "mbox.h"
#include "uart.h"

volatile unsigned int __attribute__((aligned(16))) mailbox[36];

int mbox_call(unsigned char ch) {
  // combine the message address (upper 28 bits) with channel number (lower 4
  // bits)
  unsigned int r =
      (((unsigned int)((unsigned long)&mailbox) & ~0xF) | (ch & 0xF));

  uart_print("mbox_call r: ");
  uart_hex(r);
  uart_print("\n");

  // wait until we can write to the mailbox
  do {
    asm volatile("nop");
  } while (*MBOX_STATUS_PTR & MBOX_FULL);

  *MBOX_WRITE_PTR = r;

  // now wait for the response
  while (1) {
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS_PTR & MBOX_EMPTY);

    // check if it is a response to our message
    if (r == *MBOX_READ_PTR) {
      // check if it is a valid successful response
      uart_print("mbox_call mailbox[1]: ");
      uart_hex(mailbox[1]);
      uart_print("\n");

      return mailbox[1] == MBOX_RESPONSE;
    }
  }

  return 0;
}

int get_board_revision(unsigned int *vision) {
  mailbox[0] = 7 * 4;
  mailbox[1] = MBOX_REQUEST;

  // tags begin
  mailbox[2] = MBOX_TAG_BOARD_REVISION;

  // maximum of request and response value buffer's length
  mailbox[3] = 4;

  mailbox[4] = TAG_REQUEST_CODE;

  // value buffer
  mailbox[5] = 0;
  // tags end

  mailbox[6] = MBOX_TAG_END;

  if (mbox_call(MBOX_CH_PROP) == 0) {
    return -1;
  }

  *vision = mailbox[5];

  return 0;
}

int get_arm_base_memory(unsigned int *base_addr, unsigned int *mem_size) {
  mailbox[0] = 7 * 4;
  mailbox[1] = MBOX_REQUEST;
  mailbox[2] = MBOX_TAG_GET_MEM;
  mailbox[3] = 8;
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0;
  mailbox[6] = 0;
  mailbox[7] = MBOX_TAG_END;

  for (int i = 0; i < 8; i++) {
    uart_print("mailbox ");
    uart_write(i + 0x30);
    uart_print(": ");
    uart_hex(mailbox[i]);
    uart_print("\n");
  }

  if (mbox_call(MBOX_CH_PROP) == 0) {
    return -1;
  }

  *base_addr = mailbox[5];
  *mem_size = mailbox[6];

  return 0;
}
