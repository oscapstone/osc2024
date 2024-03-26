#ifndef _MBOX_H
#define _MBOX_H

extern volatile unsigned int mbox[36]; /* a properly aligned buffer */

int mbox_call(unsigned char ch);

#endif