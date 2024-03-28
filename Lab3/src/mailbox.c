#include "mailbox.h"
#include "mini_uart.h"

int mailbox_call(volatile unsigned int *message){

  unsigned int msg = (unsigned int)(((unsigned long)message & ~0xf) | CHANNEL8);

  while(*MAILBOX_STATUS & MAILBOX_FULL);

  *MAILBOX_WRITE = msg;

  while(1){
    while(*MAILBOX_STATUS & MAILBOX_EMPTY);

    if(msg == (*MAILBOX_READ))
      return message[1] == REQUEST_SUCCESS;
  }
  // return 0;
}

void get_board_revision(){
  volatile unsigned int __attribute__((aligned(16))) mailbox[7];
  mailbox[0] = 7 * 4; // buffer size in bytes
  mailbox[1] = REQUEST_CODE;
  // tags begin
  mailbox[2] = GET_BOARD_REVISION; // tag identifier
  mailbox[3] = 4; // maximum of request and response value buffer's length.
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0; // value buffer
  // tags end
  mailbox[6] = END_TAG;

  if(mailbox_call(mailbox)){
    uart_sends("broad revision\t: ");
    uart_sendh(mailbox[5]);
    uart_sendc('\n');
  }
}

void get_arm_memory(){
  volatile unsigned int mailbox[8] __attribute__((aligned(16)));
  mailbox[0] = 8 * 4; // buffer size in bytes
  mailbox[1] = REQUEST_CODE;
  mailbox[2] = GET_ARM_MEMORY; // tag identifier
  mailbox[3] = 8; // maximum of request and response value buffer's length.
  mailbox[4] = TAG_REQUEST_CODE;
  mailbox[5] = 0; // value buffer
  mailbox[6] = 0; // value buffer
  mailbox[7] = END_TAG;
  if(mailbox_call(mailbox)){
    uart_sends("Arm memory base address\t: ");
    uart_sendh(mailbox[5]);
    uart_sends("\nArm memory size\t: ");
    uart_sendh(mailbox[6]);
    uart_sendc('\n');
  }
}