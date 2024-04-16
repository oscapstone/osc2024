#ifndef __INITRD_H__
#define __INITRD_H__

//  cpio_newc_header 
typedef struct{
	char    c_magic[6];       /* string "070701" */
	char    c_ino[8];         /* i-node number */
	char    c_mode[8];        /* File permissions and type */
  char    c_uid[8];         /* User id */
  char    c_gid[8];         /* Group id */
  char    c_nlink[8];       /* Number of links */
  char    c_mtime[8];       /* Modification time */
  char    c_filesize[8];    /* Size */
  char    c_devmajor[8];    /* device major number */
  char    c_devminor[8];    /* device minor number  */
  char    c_rdevmajor[8];   /* device major number */
  char    c_rdevminor[8];   /* device minor number  */
  char    c_namesize[8];    /* Bytes of the pathname */
  char    c_check[8];       /* Write: set to 0; Read: ignore*/
}cpio_header;

void initrd_list();
void initrd_cat(char *filename); //const char* filename
int get_initrd();
void initramfs_callback(void *node, char *propname);
void* fetch_exec(char *filename);

#endif