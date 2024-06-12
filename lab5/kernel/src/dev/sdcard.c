
#include "base.h"
#include "sdcard.h"
#include "sdhost.h"
#include "mbr.h"
#include "utils/printf.h"
#include "fs/fs.h"
#include "mm/mm.h"
#include "fs/fat32fs.h"

static int sdcard_read(U64 offset, void* buf, size_t len);
static int sdcard_write(U64 offset, const void* buf, size_t len);

int sdcard_init() {

    NS_DPRINT("[SDCARD][TRACE] initializing SD card.\n");
    // initialize the SD card on RPI
    sd_init();

    // mount

    if (vfs_mkdir(NULL, "/boot")) {
        printf("[SDCARD][ERROR] Failed to create /boot folder.\n");
        return -1;
    }
    FS_VNODE* sdcardroot = NULL;
    if (vfs_lookup(NULL, "/boot", &sdcardroot)) {
        printf("[SDCARD][ERROR] Failed to get /boot directory.\n");
        return -1;
    }
    FS_FILE_SYSTEM* fs = fs_get(FAT32_FS_NAME);
    if (!fs) {
        printf("[SDCARD][ERROR] Failed to get FAT32 FS\n");
        return -1;
    }

    FS_MOUNT* mount = kzalloc(sizeof(FS_MOUNT));
    // initiralize the read write for hardware
    mount->read = &sdcard_read;
    mount->write = &sdcard_write;
    printf("[SDCARD] mounting SD card on /boot ...\n");
    if (vfs_mount(mount, fs, sdcardroot) == -1) {
        return -1;
    }
    return 0;
}

static int sdcard_read(U64 offset, void* buf, size_t len) {
    U64 current_offset = 0;
    char tmp_buf[MBR_DEFAULT_SECTOR_SIZE];
    while (current_offset < len) {
        U64 block_offset = (offset + current_offset) / MBR_DEFAULT_SECTOR_SIZE;
        U64 buf_offset = (offset + current_offset) - (block_offset * MBR_DEFAULT_SECTOR_SIZE);
        
        U64 end_of_sector = MBR_DEFAULT_SECTOR_SIZE - buf_offset;
        size_t size = len - current_offset > end_of_sector ? end_of_sector : len - current_offset;

        sd_readblock(block_offset, tmp_buf);
        // prevent memory over copying to out of buffer size user gave.
        memcpy(tmp_buf, (void*)((U64)buf + buf_offset), size);
        buf = (char*)buf + size;
        current_offset += size;
    }
    return current_offset;
}

static int sdcard_write(U64 offset, const void* buf, size_t len) {
    U64 current_offset = 0;
    while (current_offset < len) {
        U64 block_offset = (offset + current_offset) / MBR_DEFAULT_SECTOR_SIZE;
        U64 buf_offset = (offset + current_offset) - (block_offset * MBR_DEFAULT_SECTOR_SIZE);

        U64 end_of_sector = MBR_DEFAULT_SECTOR_SIZE - buf_offset;

        size_t size = len - current_offset > end_of_sector ? end_of_sector : len - current_offset;
        sd_writeblock(block_offset, (const void*)((UPTR)buf + buf_offset));
        buf = (char*)buf + size;
        current_offset += size;
    }
    return current_offset;
}