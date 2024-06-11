#include "fs/fat32fs.hpp"

#include "fs/log.hpp"
#include "io.hpp"
#include "string.hpp"

#define FS_TYPE "fat32fs"

namespace fat32fs {

FileSystem* Vnode::fs() const {
  return static_cast<FileSystem*>(this->mount_root->fs);
}

string Vnode::_get_name(const FAT32_DirEnt* dirent) {
  string name{};
  name.reserve(sizeof(dirent->DIR_Name) + 1);

  auto name_end = dirent->file_name + sizeof(dirent->file_name) - 1;
  while (name_end != dirent->file_name and *name_end == ' ')
    name_end--;

  auto ext_end = dirent->file_ext + sizeof(dirent->file_ext) - 1;
  while (ext_end != dirent->file_ext and *ext_end == ' ')
    ext_end--;

  name += {dirent->file_name, name_end - dirent->file_name + 1};
  if (ext_end != dirent->file_ext) {
    name += ".";
    name += {dirent->file_ext, ext_end - dirent->file_ext + 1};
  }

  return name;
}

void Vnode::_load_childs() {
  auto buf = new char[BLOCK_SIZE];
  auto idx = fs()->cluster2sector(_cluster);
  FS_INFO("%s: cluster %u -> idx %u\n", __func__, _cluster, idx);

  auto beg = (FAT32_DirEnt*)(buf);
  auto end = (FAT32_DirEnt*)(buf + BLOCK_SIZE);
  auto it = end;
  for (uint32_t off = 0;; off++, it++) {
    if (it == end) {
      fs()->read_data(idx++, buf);
      it = beg;
    }
    if (it->end())
      break;
    if (it->unused())
      continue;
    if (it->is_long_name()) {
      // khexdump(it, sizeof(*it), "long name");
      continue;
    }
    // khexdump(it->DIR_Name, sizeof(it->DIR_Name), "name");
    auto filename = _get_name(it);
    // kprintf("name: ");
    // kprint(filename);
    // kprintf("\n");
    if (filename == "." or filename == "..")
      continue;
    auto new_vnode = new Vnode{this->mount_root, it, off};
    this->set_child(filename.data(), new_vnode);
  }

  delete[] buf;
}

bool Vnode::_resize(size_t new_size) {
  _modified = true;
  _content.resize(new_size);
  _filesize = new_size;
  return true;
}

char* Vnode::_write_ptr() {
  _modified = true;
  return _content.data();
}

const char* Vnode::_read_ptr() {
  if (not _load) {
    _load = true;
    _content.resize(_filesize);
    auto idx = fs()->cluster2sector(_cluster);
    FS_INFO("_load_content: size %u from idx %u\n", _filesize, idx);
    fs()->read_data(idx, _content.data(), _filesize);
  }
  return _content.data();
}

Vnode::Vnode(const ::Mount* mount)
    : Base{mount, kDir},
      _offset{ROOT_OFFSET},
      _cluster{fs()->bpb->BPB_RootClus} {
  FS_INFO("vnode sync %d / %u / %p\n", isDir(), _cluster, this);
  _load_childs();
}

Vnode::Vnode(const ::Mount* mount, FAT32_DirEnt* dirent, uint32_t off)
    : Base{mount,
           has(dirent->DIR_Attr, FILE_Attrs::ATTR_DIRECTORY) ? kDir : kFile},
      _offset{off},
      _cluster{dirent->FstClus()} {
  switch (type) {
    case kFile:
      _filesize = dirent->DIR_FileSize;
      break;
    case kDir:
      _load_childs();
      break;
  }
}

Vnode::Vnode(const ::Mount* mount, filetype type)
    : Base{mount, type},
      _load{true},
      _offset{NO_OFFSET},
      _cluster{NO_CLUSTER},
      _filesize(0),
      _content{""} {}

bool FileSystem::init = false;

FileSystem::FileSystem() {
  if (not init) {
    init = true;
    sd_init();
  }

  block_buf = new char[BLOCK_SIZE];

  mbr = new MBR;
  sector_0_off = 0;
  read_data(0, mbr);
  FS_HEXDUMP("MBR", mbr, sizeof(MBR));

  if (not mbr->valid()) {
    FS_WARN("disk is invalid MBR (sig = %x%x)\n", mbr->sig[0], mbr->sig[1]);
    return;
  }

  if (mbr->entry[0].active) {
    FS_INFO("first partition entry is active\n");
  }

  if (mbr->entry[0].type != PARTITION_ID_FAT32_CHS) {
    FS_WARN("first partition entry isn't FAT32 with CHS (type = 0x%x)\n",
            mbr->entry[0].type);
    return;
  }

  FS_HEXDUMP("first partition entry", &mbr->entry[0], 16);

  FS_INFO("first sector 0x%x\n", mbr->entry[0].first_sector_LBA);
  auto print_CHS = [](const char* name, CHS chs) {
    FS_INFO("%s %u / %u / %u\n", name, chs.C, chs.H, chs.S);
  };
  print_CHS("first sector", mbr->entry[0].first_sector_CHS);
  print_CHS("last sector", mbr->entry[0].last_sector_CHS);

  sector_0_off = mbr->entry[0].first_sector_LBA;

  bpb = new FAT_BPB;
  read_data(0, bpb);
  FS_HEXDUMP("FAT_BPB", bpb, sizeof(FAT_BPB));

  strncpy(label, bpb->BS_VolLab, sizeof(bpb->BS_VolLab));
  FS_INFO("volumn label: '%s'\n", label);

  if (strncmp("FAT32   ", bpb->BS_FilSysType, sizeof(bpb->BS_FilSysType))) {
    FS_WARN("invalid type '%s'\n", bpb->BS_FilSysType);
    return;
  }

  if (bpb->BPB_RootEntCnt != 0) {
    FS_WARN("BPB_RootEntCnt is not zero (%d)\n", bpb->BPB_RootEntCnt);
    return;
  }

  RootDirSectors = ((bpb->BPB_RootEntCnt * 32) + (bpb->BPB_BytsPerSec - 1)) /
                   bpb->BPB_BytsPerSec;
  FS_INFO("RootDirSectors = %d\n", RootDirSectors);

  if (bpb->BPB_FATSz16 != 0) {
    FS_WARN("BPB_FATSz16 is not zero (%d)\n", bpb->BPB_FATSz16);
    return;
  }

  FATSz = bpb->BPB_FATSz32;

  read_block(bpb->BPB_RsvdSecCnt);
  FS_INFO("first FAT\n");
  FS_HEXDUMP("FAT", block_buf, BLOCK_SIZE);

  FirstDataSector =
      bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) + RootDirSectors;
  FS_INFO("FirstDataSector = %d\n", FirstDataSector);

  if (bpb->BPB_TotSec16 != 0) {
    FS_WARN("BPB_TotSec16 is not zero (%d)\n", bpb->BPB_TotSec16);
    return;
  }

  TotSec = bpb->BPB_TotSec32;
  FS_INFO("TotSec = %d\n", TotSec);
  DataSec = TotSec -
            (bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * FATSz) + RootDirSectors);
  FS_INFO("DataSec = %d\n", DataSec);
  CountofClusters = DataSec / bpb->BPB_SecPerClus;
  FS_INFO("CountofClusters = %d\n", CountofClusters);

  if (CountofClusters < 4085) {
    FS_WARN("Volume is FAT12\n");
    return;
  } else if (CountofClusters < 65525) {
    FS_WARN("Volume is FAT16\n");
    return;
  } else {
    FS_INFO("Volume is FAT32\n");
  }

  fsinfo = new FAT32_FSInfo;
  read_data(1, fsinfo);

  if (not fsinfo->valid()) {
    FS_WARN("fsinfo is invalid (sig = 0x%x)\n", fsinfo->FSI_LeadSig);
    return;
  }

  FS_INFO("root cluster %d -> %d\n", bpb->BPB_RootClus,
          cluster2sector(bpb->BPB_RootClus));
  read_block(cluster2sector(bpb->BPB_RootClus));
  FS_HEXDUMP("root cluster", block_buf, BLOCK_SIZE);
}

::Vnode* FileSystem::mount(const ::Mount* mount_root) {
  return new Vnode{mount_root};
}

uint32_t Vnode::_find_dir_off() {
  auto buf = new char[BLOCK_SIZE];
  auto idx = fs()->cluster2sector(_cluster);

  auto beg = (FAT32_DirEnt*)(buf);
  auto end = (FAT32_DirEnt*)(buf + BLOCK_SIZE);
  auto it = end;
  uint32_t off = 0;
  for (;; off++, it++) {
    if (it == end) {
      fs()->read_data(idx++, buf);
      it = beg;
    }
    if (it->unused())
      break;
    if (it->end()) {
      it->set_unused();
      fs()->write_data(idx - 1, buf);
      break;
    }
  }

  delete[] buf;

  return off;
}

FAT32_DirEnt Vnode::_get_dirent(const char* name) const {
  FAT32_DirEnt ent{
      .DIR_Name{},
      .DIR_Attr =
          isDir() ? FILE_Attrs::ATTR_DIRECTORY : FILE_Attrs::ATTR_ARCHIVE,
      .DIR_NTRes{},
      .DIR_CrtTimeTenth = 0,
      .DIR_CrtTime = 0,
      .DIR_CrtDate = 0x0011,
      .DIR_LstAccDate = 0x0011,
      .DIR_FstClusHI = (uint16_t)(_cluster >> 16),
      .DIR_WrtTime = 0,
      .DIR_WrtDate = 0x0011,
      .DIR_FstClusLO = (uint16_t)(_cluster & MASK(16)),
      .DIR_FileSize = _filesize,
  };
  memset(ent.DIR_Name, ' ', sizeof(ent.DIR_Name));
  size_t i = 0;
  for (; name[i] and name[i] != '.'; i++) {
    if (i == sizeof(ent.file_name)) {
      FS_WARN("%s: filename too long (%s)\n", __func__, name);
      break;
    }
    ent.file_name[i] = name[i];
  }
  if (name[i] == '.') {
    for (size_t j = i + 1; name[j]; j++) {
      if (j == sizeof(ent.file_ext)) {
        FS_WARN("%s: file ext too long (%s)\n", __func__, name);
        break;
      }
      ent.file_ext[j - i - 1] = name[j];
    }
  }
  return ent;
}

void Vnode::_sync() {
  FS_INFO("vnode sync %d / %u / %p\n", isDir(), _cluster, this);
  if (isDir()) {
    if (_cluster == NO_CLUSTER) {
      // TODO
      FS_WARN("create dir not support\n");
      return;
    }
    for (auto child : childs()) {
      if (child.node == this or child.node == parent())
        continue;
      auto vnode = static_cast<Vnode*>(child.node);
      FS_INFO("sync %s\n", child.name);
      vnode->_sync();
      if (vnode->_offset == NO_OFFSET) {
        vnode->_offset = _find_dir_off();
        FS_INFO("'%s' offset %u\n", child.name, vnode->_offset);
        vnode->_modified = true;
      }
      if (vnode->_modified) {
        auto ent = vnode->_get_dirent(child.name);
        khexdump(&ent, sizeof(ent));
        fs()->write_data(fs()->cluster2sector(_cluster), &ent,
                         vnode->_offset * sizeof(FAT32_DirEnt));
        vnode->_modified = false;

        fs()->read_block(fs()->cluster2sector(_cluster));
        khexdump(fs()->block_buf, BLOCK_SIZE);
      }
    }
  } else {
    if (_cluster == NO_CLUSTER) {
      // TODO
    }
    // fs()->write_data(_cluster, _content.data(), _content.size());
  }
}

void FileSystem::sync(const Mount* mount_root) {
  static_cast<Vnode*>(mount_root->root)->_sync();
}

};  // namespace fat32fs
