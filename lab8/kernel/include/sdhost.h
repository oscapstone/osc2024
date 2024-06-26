#ifndef _SDHOST_H_
#define _SDHOST_H_

void readblock(int block_idx, void* buf);
void writeblock(int block_idx, void* buf);
void sd_init();

#endif /* _SDHOST_H_ */
