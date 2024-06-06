#include "fs/uartfs.hpp"

#include "io.hpp"

namespace uartfs {

int File::write(const void* buf, size_t len) {
  return kwrite(buf, len);
}

int File::read(void* buf, size_t len) {
  return kread(buf, len);
}

}  // namespace uartfs
