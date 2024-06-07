#pragma once

void sd_init();

void sd_readblock(int block_idx, void* buf);
void sd_writeblock(int block_idx, const void* buf);