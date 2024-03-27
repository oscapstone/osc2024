#ifndef _DEF_INITRAMFS
#define _DEF_INITRAMFS

typedef struct
{
    /* data */
} initramfs_t;

typedef struct
{
    /* data */
} cpio_object_t;



void parse_initramfs(void);
void list_initramfs(void);
void cat_initramfs(void);

#endif
