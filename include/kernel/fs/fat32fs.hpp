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

class Vnode final : public ::VnodeImplRW<Vnode, File> {
  static constexpr uint32_t NO_CLUSTER = 0;
  using Base = ::VnodeImplRW<Vnode, File>;

  friend File;
  friend FileSystem;

  FileSystem* fs() const;

  bool modified = false;
  uint32_t _filesize, _cluster;
  bool _load = false;
  string _content;

  static string _get_name(const FAT32_DirEnt* dirent);
  void _load_childs();

  bool _resize(size_t new_size);
  char* _write_ptr();
  const char* _read_ptr();
  void _sync();

 public:
  Vnode(const ::Mount* mount);
  Vnode(const ::Mount* mount, FAT32_DirEnt* dirent);
  Vnode(const ::Mount* mount, filetype type);
  virtual ~Vnode() = default;
  virtual long filesize() const {
    return _filesize;
  }
};

class File final : public ::FileImplRW<Vnode, File> {
  virtual bool resize(size_t new_size) {
    return get()->_resize(new_size);
  }
  virtual char* write_ptr() {
    get()->modified = true;
    return get()->_write_ptr();
  }
  virtual const char* read_ptr() {
    return get()->_read_ptr();
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
  virtual void sync(const Mount* mount_root);
};

}  // namespace fat32fs
