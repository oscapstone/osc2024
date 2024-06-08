#pragma once

#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"
#include "fs/fat.hpp"
#include "fs/mbr.hpp"
#include "fs/sdhost.hpp"

namespace fat32fs {

class Vnode;
class File;
class FileSystem;

class Vnode final : public ::VnodeImpl<Vnode, File> {
  int _filesize;

 public:
  using ::VnodeImpl<Vnode, File>::VnodeImpl;
  virtual ~Vnode() = default;
  virtual long filesize() const {
    return _filesize;
  }
};

class File final : public ::FileImplRW<Vnode, File> {
  virtual bool resize(size_t new_size) {
    return false;
  }
  virtual char* write_ptr() {
    return nullptr;
  }
  virtual char* read_ptr() {
    return nullptr;
  }

 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;
};

class FileSystem final : public ::FileSystem {
  ::Vnode* root = nullptr;
  static bool init;
  char* block_buf = nullptr;
  uint32_t sector_0_off;
  MBR* mbr;
  FAT_BPB* bpb;
  char label[12]{};
  uint32_t RootDirSectors, FATSz, FirstDataSector, TotSec, DataSec,
      CountofClusters;
  FAT32_FSInfo* fsinfo;

  void read_block(uint32_t idx) {
    readblock(sector_0_off + idx, block_buf);
  }

  template <typename T>
  void read_data(uint32_t idx, T* t) {
    read_block(idx);
    memcpy(t, block_buf, sizeof(T));
  }

  template <>
  void read_data(uint32_t idx, char* t) {
    read_block(idx);
    memcpy(t, block_buf, BLOCK_SIZE);
  }

  uint32_t cluster2sector(uint32_t N) {
    return ((N - 2) * bpb->BPB_SecPerClus) + FirstDataSector;
  };

 public:
  FileSystem();

  virtual const char* name() const {
    return "fat32fs";
  }

  virtual ::Vnode* mount(const Mount* mount_root) {
    return root;
  }
};

}  // namespace fat32fs
