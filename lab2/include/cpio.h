#pragma once

struct cpio_newc_header {
	//file metadata
	char c_magic[6]; // Magic number identifying the CPIO format
	char c_ino[8]; // Inode number
	char c_mode[8]; // File mode and type
	char c_uid[8]; // User ID of file owner
	char c_gid[8]; // Group ID of file owner
	char c_nlink[8]; // Number of hard links
	char c_mtime[8]; // Modification time of file
	char c_filesize[8]; // Size of file (in hexadecimal)
	char c_devmajor[8]; // Major device number (for device files)
	char c_devminor[8]; // Minor device number (for device files)
	char c_rdevmajor
		[8]; // Major device number for the device file node referenced by the symlink
	char c_rdevminor
		[8]; // Minor device number for the device file node referenced by the symlink
	char c_namesize[8]; // Size of filename (in hexadecimal)
	char c_check[8]; // Checksum
};
// Link: https://manpages.ubuntu.com/manpages/bionic/man5/cpio.5.html

void cpio_ls();
void cpio_cat();
