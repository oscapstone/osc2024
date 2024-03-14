#include "header/mailbox.h"

//volatile: 優化器在用到這個變數時必須每次重新從虛擬記憶體中讀取這個變數的值，而不是使用儲存在暫存器裡的備份。
//__attribute__: GCC的一種編譯器指令
//__attribute__((aligned(n))): 内存对齐，指定内存对齐byte
//sizeof(mailbox[0]) = 4 (unsinged int)
volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

int mailbox_call(unsigned char ch) 
{
    //&~0xF => set lower 4 bit to 0 (&0 => all set to 0)
    //step 1
    unsigned int r = (((unsigned int)((unsigned long)&mailbox)&~0xF) | (ch & 0xF)); //combie mailbox[31:4]+MBOX_CH_PROP[3:0]
    /* wait until we can write to the mailbox */
    //step 2
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    //step 3
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        //step 4
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        //step 6
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            //step 5
            return mailbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

//HWinfo
void HardwareInformation(){
    // get the board's unique serial number with a mailbox call
    mailbox[0] = 8 * 4;        // length of the message
    mailbox[1] = MBOX_REQUEST; // this is a request message

    mailbox[2] = MBOX_TAG_GETSERIAL; // get serial number command
    mailbox[3] = 8;                  // buffer size
    mailbox[4] = 8;
    mailbox[5] = 0; // clear output buffer
    mailbox[6] = 0;

    mailbox[7] = MBOX_TAG_LAST;
    mailbox_call(MBOX_CH_PROP);
}

//board_revision
void get_board_revision(){
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = MBOX_REQUEST;    
    // tags begin
    //mailbox[2] = MBOX_TAG_GETSERIAL;
    mailbox[2] = MBOX_TAG_GETBOARD; // tag identifier
    mailbox[3] = 4;          // maximum of request and response value buffer's length.
    //mailbox[4] = TAG_REQUEST_CODE;
    mailbox[4] = 4;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = MBOX_TAG_LAST;
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, we should implement it following the 6 steps provided above.
}

//ARM memory base address
void get_arm_mem(){
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = MBOX_REQUEST;    
    // tags begin
    //mailbox[2] = MBOX_TAG_GETSERIAL;
    mailbox[2] = MBOX_TAG_GETARMMEM; // tag identifier
    mailbox[3] = 8;          // maximum of request and response value buffer's length.
    mailbox[4] = 8;
    //mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = MBOX_TAG_LAST;
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, we should implement it following the 6 steps provided above.
}