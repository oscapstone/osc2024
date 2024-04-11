#include "peripherals/mbox.h"

#include "utli.h"

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mbox[36];

int mbox_call(unsigned char ch) {
  unsigned int r = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
  /* wait until we can write to the mailbox */
  do {
    asm volatile("nop");
  } while (get(MBOX_STATUS) & MBOX_FULL);
  /* write the address of our message to the mailbox with channel identifier */

  set(MBOX_WRITE, r);
  /* now wait for the response */

  do {
    asm volatile("nop");
  } while (get(MBOX_STATUS) & MBOX_EMPTY);

  while (1) {
    /* is there a response? */
    do {
      asm volatile("nop");
    } while (get(MBOX_STATUS) & MBOX_EMPTY);
    /* is it a response to our message? */
    if (r == get(MBOX_READ)) /* is it a valid successful response? */
      return (mbox[1] == MBOX_CODE_BUF_RES_SUCC);
  }
  return 0;
}
