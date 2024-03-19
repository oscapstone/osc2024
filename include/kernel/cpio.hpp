#pragma once

#include "string.hpp"

struct cpio_newc_header {
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

  inline bool valid() {
    return !memcmp(c_magic, MAGIC, sizeof(MAGIC) - 1);
  }
  inline bool isend() {
    return !strcmp(name_ptr(), ENDFILE);
  }

  inline int mode() {
    return strtol(c_mode, nullptr, 16, sizeof(c_mode));
  }
  inline bool isdir() {
    return (mode() & F_MASK) == F_DIR;
  }

  inline int namesize() {
    return strtol(c_namesize, nullptr, 16, sizeof(c_namesize));
  }
  inline int filesize() {
    return strtol(c_filesize, nullptr, 16, sizeof(c_filesize));
  }
  inline char* name_ptr() {
    return _name_ptr;
  }
  inline char* file_ptr() {
    return name_ptr() + namesize() + 2;
  }
  string_view name() {
    return {name_ptr(), namesize() - 1};
  }
  string_view file() {
    return {file_ptr(), filesize()};
  }
  cpio_newc_header* next();
};

class CPIO {
 public:
  class iterator {
   public:
    iterator(char* header) : hedaer_((cpio_newc_header*)header) {}

    inline iterator& operator++() {
      hedaer_ = hedaer_->next();
      return *this;
    }

    inline iterator operator++(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }

    inline cpio_newc_header* operator*() const {
      return hedaer_;
    }

    inline cpio_newc_header* operator->() const {
      return hedaer_;
    }

    bool operator==(const iterator& other) const {
      return other.hedaer_ == hedaer_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

   private:
    cpio_newc_header* hedaer_;
  };

 private:
  char* const cpio_addr_;

 public:
  CPIO(char* cpio_addr) : cpio_addr_(cpio_addr) {}
  iterator begin() {
    return cpio_addr_;
  }
  iterator end() {
    return nullptr;
  }
  cpio_newc_header* find(const char* name);
};

extern CPIO cpio;
