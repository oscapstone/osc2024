#include "cpio.hpp"

const cpio_newc_header* cpio_newc_header::next() const {
  const char* nxt = align<4>(file().end());
  auto hdr = (const cpio_newc_header*)nxt;
  if (hdr->valid() and not hdr->isend())
    return hdr;
  return nullptr;
}

const cpio_newc_header* CPIO::find(const char* name) const {
  for (auto it = begin(); it != end(); it++) {
    if (it->name() == name)
      return *it;
  }
  return nullptr;
}
