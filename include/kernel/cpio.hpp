#pragma once

#include "string.hpp"
#include "util.hpp"

// ref: https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
struct __attribute__((__packed__)) cpio_newc_header {
  char c_magic[6];
  char c_ino[8];
  char c_mode[8];
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];
  char c_mtime[8];
  char c_filesize[8];
  char c_devmajor[8];
  char c_devminor[8];
  char c_rdevmajor[8];
  char c_rdevminor[8];
  char c_namesize[8];
  char c_check[8];
  char _name_ptr[];

  // clang-format off
  static constexpr int IALLOC1  = 0100000; // IALLOC flag - irrelevant to cpio.
  static constexpr int F_MASK   = 0060000; // This masks the file type bits.
  static constexpr int F_DIR    = 0040000; // File type value for directories.
  static constexpr int F_CDEV   = 0020000; // File type value for character special devices.
  static constexpr int F_BDEV   = 0060000; // File type value for block special devices.
  static constexpr int IALLOC2  = 0010000; // ILARG flag - irrelevant to cpio.
  static constexpr int SUID     = 0004000; // SUID bit.
  static constexpr int SGID     = 0002000; // SGID bit.
  static constexpr int STICKY   = 0001000; // Sticky bit.
  static constexpr int RWXPERM  = 0000777; // The lower 9 bits  specify  read/write/execute  permis-
  static constexpr char ENDFILE[] = "TRAILER!!!";
  static constexpr char MAGIC[] = "070701";
  // clang-format on

  bool valid() const {
    return !memcmp(c_magic, MAGIC, sizeof(MAGIC) - 1);
  }
  bool isend() const {
    return !strcmp(name_ptr(), ENDFILE);
  }

  int mode() const {
    return strtol(c_mode, nullptr, 16, sizeof(c_mode));
  }
  bool isdir() const {
    return (mode() & F_MASK) == F_DIR;
  }

  int namesize() const {
    return strtol(c_namesize, nullptr, 16, sizeof(c_namesize));
  }
  int filesize() const {
    return strtol(c_filesize, nullptr, 16, sizeof(c_filesize));
  }
  const char* name_ptr() const {
    return _name_ptr;
  }
  const char* file_ptr() const {
    return align<4>(name_ptr() + namesize());
  }
  string_view name() const {
    return {name_ptr(), namesize() - 1};
  }
  string_view file() const {
    return {file_ptr(), filesize()};
  }
  const cpio_newc_header* next() const;
};

class CPIO {
 public:
  class iterator {
   public:
    iterator(const char* header) : hedaer_((const cpio_newc_header*)header) {}
    iterator& operator++() {
      hedaer_ = hedaer_->next();
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    const cpio_newc_header* operator*() const {
      return hedaer_;
    }
    const cpio_newc_header* operator->() const {
      return hedaer_;
    }
    bool operator==(const iterator& other) const {
      return other.hedaer_ == hedaer_;
    }
    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

   private:
    const cpio_newc_header* hedaer_;
  };

 private:
  char* cpio_addr_ = nullptr;

 public:
  bool init(char* cpio_addr) {
    cpio_addr_ = cpio_addr;
    if (!begin()->valid())
      return false;
    return true;
  }
  iterator begin() const {
    return cpio_addr_;
  }
  iterator end() const {
    return nullptr;
  }
  const cpio_newc_header* find(const char* name) const;
};
