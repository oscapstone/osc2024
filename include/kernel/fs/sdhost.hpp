#pragma once

constexpr int BLOCK_SIZE = 512;

void sd_init();
void readblock(int block_idx, void* buf);
void writeblock(int block_idx, void* buf);
