#ifndef CPIO_H
#define CPIO_H


#define CPIO_MAGIC        "070701"
#define CPIO_MAGIC_FOOTER "TRAILER!!!"

#ifdef QEMU
#define CPIO_ADDR         0x8000000
#else
#define CPIO_ADDR         0x20000000
#endif

/* CPIO archive format
 * https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
 */


/*
 * CPIO archive will be stored like this:
 *
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |       (file name)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |       (file data)        |
 * +--------------------------+
 * |  0 padding(4-byte align) |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |            .             |
 * |            .             |
 * |            .             |
 * +--------------------------+
 * |  struct cpio_newc_header |
 * +--------------------------+
 * |        TRAILER!!!        |
 * +--------------------------+
 */

/* CPIO new ASCII format
 * The  "new"  ASCII format uses 8-byte hexadecimal fields for all numbers
 */
struct cpio_newc_header {
    char c_magic[6];  // "070701"
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
};

enum cpio_mode { CPIO_LIST, CPIO_CAT };

void ls(void);
void cat(char*);

#endif /* CPIO_H */
