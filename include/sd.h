#ifndef __SD_H__
#define __SD_H__

#ifndef BLOCK_SIZE
#define BLOCK_SIZE                 (512)
#endif

void sd_init();
void writeblock(int block_idx, void* buf);
void readblock(int block_idx, void* buf);

void print_block(int block_idx);
void print_mbr_info(void);

#endif // __SD_H__