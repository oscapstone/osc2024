#include "cpio.hpp"

cpio_newc_header* cpio_newc_header::next() {
  char* nxt = isdir() ? name().end() : file().end();

  for (int i = 0; i < (int)sizeof(cpio_newc_header); i++) {
    auto hdr = (cpio_newc_header*)(nxt + i);
    if (hdr->valid()) {
      return hdr->isend() ? nullptr : hdr;
    }
  }
  return nullptr;
}
