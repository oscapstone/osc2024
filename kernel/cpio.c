#include "cpio.h"
#include "malloc.h"
#include "string.h"
#include "uart.h"

cpio_meta_t *init_cpio_meta()
{
    cpio_meta_t *meta = (cpio_meta_t *) malloc(sizeof(cpio_meta_t));

    meta->filename = NULL;
    meta->content = NULL;
    meta->next = NULL;
    meta->prev = NULL;

    return meta;
}
