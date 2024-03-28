#ifndef _CPIO_H
#define _CPIO_H

struct cpio_newc_header;

static unsigned long align_up(unsigned long n, unsigned long align);
int cpio_parse_header(struct cpio_newc_header *archive, char **filename, unsigned int *_filesize, void **data,
                      struct cpio_newc_header **next);
void cpio_ls();
void cpio_cat(char *file);

#endif