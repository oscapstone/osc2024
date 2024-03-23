#include "cpio.hpp"

const cpio_newc_header* cpio_newc_header::next() const {
  const char* nxt = isdir() ? name().end() : file().end();

  for (int i = 0; i < (int)sizeof(cpio_newc_header); i++) {
    auto hdr = (const cpio_newc_header*)(nxt + i);
    if (hdr->valid()) {
      return hdr->isend() ? nullptr : hdr;
    }
  }
  return nullptr;
}

const cpio_newc_header* CPIO::find(const char* name) const {
  for (auto it = begin(); it != end(); it++) {
    if (it->name() == name)
      return *it;
  }
  return nullptr;
}
