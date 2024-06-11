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
  static constexpr uint32_t NO_OFFSET = (uint32_t)-1;
  static constexpr uint32_t ROOT_OFFSET = (uint32_t)-2;
  static constexpr uint32_t NO_CLUSTER = 0;
  using Base = ::VnodeImplRW<Vnode, File>;

  friend File;
  friend FileSystem;

  FileSystem* fs() const;

  bool _modified = false, _load = false;
  uint32_t _offset, _cluster, _filesize;
  string _content;

  static string _get_name(const FAT32_DirEnt* dirent);
  void _load_childs();

  bool _resize(size_t new_size);
  char* _write_ptr();
  const char* _read_ptr();
  uint32_t _find_dir_off();
  FAT32_DirEnt _get_dirent(const char* name) const;
  void _sync();

 public:
  Vnode(const ::Mount* mount);
  Vnode(const ::Mount* mount, FAT32_DirEnt* dirent, uint32_t off);
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
    get()->_modified = true;
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
    SaveDAIF saveDAIF;
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

  void write_block(uint32_t idx) {
    SaveDAIF saveDAIF;
    writeblock(sector_0_off + idx, block_buf);
  }

  void write_data(uint32_t idx, void* t, size_t size) {
    auto s = (char*)t;
    while (size > 0) {
      auto cur_size = size < BLOCK_SIZE ? size : BLOCK_SIZE;
      memcpy(block_buf, s, cur_size);
      memset(block_buf + cur_size, 0, BLOCK_SIZE - cur_size);
      write_block(idx);
      idx++;
      size -= cur_size;
      s += cur_size;
    }
  }

  void write_data(uint32_t idx, void* t, size_t offset = 0,
                  size_t size = BLOCK_SIZE) {
    // TODO: handle case when size > BLOCK_SIZE
    auto off = offset % BLOCK_SIZE, adj = offset / BLOCK_SIZE;
    auto s = (char*)t;
    read_block(idx + adj);
    memcpy(block_buf + off, s, size);
    write_block(idx + adj);
  }

  template <typename T>
  void write_data(uint32_t idx, T* t, size_t offset = 0) {
    write_data(idx, t, offset, sizeof(T));
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
