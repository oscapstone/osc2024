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
  friend File;

  FileSystem* fs() const;

  uint32_t _filesize, _cluster;
  bool _load;
  string _content;

  static string _get_name(const FAT32_DirEnt* dirent);
  void _load_childs(size_t cluster);
  const char* _load_content();

 public:
  Vnode(const ::Mount* mount);
  Vnode(const ::Mount* mount, FAT32_DirEnt* dirent);
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
  virtual const char* read_ptr() {
    return get()->_load_content();
  }

 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;
};

class FileSystem final : public ::FileSystem {
  friend Vnode;

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

  void read_data(uint32_t idx, void* t, size_t size) {
    auto s = (char*)t;
    while (size > 0) {
      read_block(idx);
      idx++;
      auto cur_size = size < BLOCK_SIZE ? size : BLOCK_SIZE;
      memcpy(s, block_buf, cur_size);
      size -= cur_size;
      s += cur_size;
    }
  }

  template <typename T>
  void read_data(uint32_t idx, T* t) {
    read_data(idx, t, sizeof(T));
  }

  template <>
  void read_data(uint32_t idx, char* t) {
    read_data(idx, t, BLOCK_SIZE);
  }

  uint32_t cluster2sector(uint32_t N) {
    return ((N - 2) * bpb->BPB_SecPerClus) + FirstDataSector;
  };

 public:
  FileSystem();

  virtual const char* name() const {
    return "fat32fs";
  }

  virtual ::Vnode* mount(const ::Mount* mount_root);
};

}  // namespace fat32fs
