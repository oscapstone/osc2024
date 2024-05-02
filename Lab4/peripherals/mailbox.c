#include "mailbox.h"
#include "mini_uart.h"

//  Check messageBuffer status.
volatile unsigned int* const mailboxStatus = MAILBOX_STATUS;
volatile unsigned int* const mailboxRead = MAILBOX_READ;
volatile unsigned int* const mailboxWrite = MAILBOX_WRITE;

// Message buffer must be 16-byte aligned.
unsigned int messageBuffer[8] __attribute__((aligned(16)));


void mailbox_write(unsigned int channel, unsigned int data) {
    // Wait for the mailbox to become not full.
    while (*mailboxStatus & MAILBOX_FULL);
    // Write the data (with channel) to the mailbox.
    *mailboxWrite = data | (channel & 0xF);
}

// Nothing is written in mailbox for channel 8. However, when using other channels this value might have its
// uses.
unsigned int mailbox_read(unsigned int channel) {
    unsigned int data;

    do {
        // If mailbox isn't empty, read it.
        while (*mailboxStatus & MAILBOX_EMPTY);
        data = *mailboxRead;
        
    } while (channel != (data & 0xF));  // Check whether the response is meant for the channel.

    return (data & 0xFFFFFFF0);
}

void get_board_revision() {
    messageBuffer[0] = sizeof(messageBuffer); // buffer size in bytes
    messageBuffer[1] = MAILBOX_REQUEST;
    // tags begin
    // Tag identifier: Specifies the type of request or information being queried or configured.
    messageBuffer[2] = MAILBOX_TAG_GETBOARD;

    // Value buffer size: Indicates the size of the value buffer that follows, specifying how much space is allocated
    // for the data related to this tag. It includes space for both the request data (if any) and
    // the response data.
    // Allocates 4 bytes, since board revision number is a 32-bit value.
    messageBuffer[3] = 4;

    // Request/Response Size: Specifies the size of the request data(which may be 0 if no request data is needed),
    // Upon processing the message, the GPU updates this field to reflect the size of the response data it has placed
    // in the value buffer.
    messageBuffer[4] = TAG_REQUEST_CODE;

    // Value Buffer: Contains the actual data for the request or response. The content and format of this data vary
    // depending on the tag identifier. For a request, this might be parameters or configuration data; for a response,
    // it's the information being returned by the GPU.
    messageBuffer[5] = 0;
    // tags end
    // A zero value marks the end of the tags in the message. This signals to the GPU that there are no more tags to
    // process in this message.
    messageBuffer[6] = MAILBOX_TAG_END;
    messageBuffer[7] = 0; // Unused for requesting board revision number.

    // Here we're passing the ADDRESS of messageBuffer, not the actual data stored in the buffer!
    // 0xC0000000 is for bypassing the CPU cache, ensuring that the mailbox message is READ DIRECTLY from memory by the GPU. 
    mailbox_write(MAILBOX_CH_PROP, (unsigned int)(unsigned long)messageBuffer | 0xC0000000);

    // Wait until the messageBuffer is available.
    while (messageBuffer[1] != MAILBOX_RESPONSE_SUCCESS);

    uart_send_string("Board revision: ");
    uart_send_uint(messageBuffer[5]);   // it should be 0xa020d3 for rpi3 b+
}

void get_arm_memory() {
    messageBuffer[0] = sizeof(messageBuffer); // buffer size in bytes
    messageBuffer[1] = MAILBOX_REQUEST;
    // tags begin
    // Tag identifier: Specifies the type of request or information being queried or configured.
    messageBuffer[2] = MAILBOX_TAG_GETARMMEM;

    // Value buffer size: Indicates the size of the value buffer that follows, specifying how much space is allocated
    // for the data related to this tag. It includes space for both the request data (if any) and
    // the response data.
    // Allocates 8 bytes, containing both base address and size.
    messageBuffer[3] = 8;

    // Request/Response Size: Specifies the size of the request data(which may be 0 if no request data is needed),
    // Upon processing the message, the GPU updates this field to reflect the size of the response data it has placed
    // in the value buffer.
    messageBuffer[4] = TAG_REQUEST_CODE;

    // Value Buffer: Contains the actual data for the request or response. The content and format of this data vary
    // depending on the tag identifier. For a request, this might be parameters or configuration data; for a response,
    // it's the information being returned by the GPU.
    messageBuffer[5] = 0;
    messageBuffer[6] = 0;
    // tags end
    // A zero value marks the end of the tags in the message. This signals to the GPU that there are no more tags to
    // process in this message.
    messageBuffer[7] = MAILBOX_TAG_END;

    // Here we're passing the ADDRESS of messageBuffer, not the actual data stored in the buffer!
    // 0xC0000000 is for bypassing the CPU cache, ensuring that the mailbox message is READ DIRECTLY from memory by the GPU. 
    mailbox_write(MAILBOX_CH_PROP, (unsigned int)(unsigned long)messageBuffer | 0xC0000000);
    // Wait until the messageBuffer is available.
    while (messageBuffer[1] != MAILBOX_RESPONSE_SUCCESS);

    uart_send_string("ARM base memory address: 0x");
    uart_send_uint(messageBuffer[5]);
    uart_send_string("\r\n");
    uart_send_string("ARM memory size: 0x");
    uart_send_uint(messageBuffer[6]);
}