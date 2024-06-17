#include "bcm2837/rpi_mbox.h"
#include "vfs.h"
#include "string.h"
#include "sched.h"
#include "exception.h"
#include "uart1.h"
#include "vfs_dev_framebuffer.h"
#include "stdio.h"

extern void *CPIO_DEFAULT_START;
extern thread_t *curr_thread;
extern thread_t threads[PIDMAX + 1];

int open(trapframe_t *tpf, const char *pathname, int flags)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);

    uart_sendlinek("file name : %s\n", pathname);
    uart_sendlinek("curr_working_dir : %s\n", curr_thread->curr_working_dir);
    uart_sendlinek("abs_path : %s\n", abs_path);

    // update abs_path
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    for (int i = 0; i < MAX_FD; i++)
    {
        // find a usable fd
        if (!curr_thread->file_descriptors_table[i])
        {
            // uart_sendlinek("i : %d\n", i);
            // while (1)
            //     ;

            if (vfs_open(abs_path, flags, &curr_thread->file_descriptors_table[i]) != 0)
            {
                break;
            }

            tpf->x0 = i;
            return i;
        }
    }

    tpf->x0 = -1;
    return -1;
}

int close(trapframe_t *tpf, int fd)
{
    // find an opened fd
    if (curr_thread->file_descriptors_table[fd])
    {
        vfs_close(curr_thread->file_descriptors_table[fd]);
        curr_thread->file_descriptors_table[fd] = 0;
        tpf->x0 = 0;
        return 0;
    }

    tpf->x0 = -1;
    return -1;
}

long write(trapframe_t *tpf, int fd, const void *buf, unsigned long count)
{
    // uart_sendlinek("fd : %d\n", fd);
    if (curr_thread->file_descriptors_table[fd])
    {
        tpf->x0 = vfs_write(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }
    tpf->x0 = -1;
    return tpf->x0;
}

long read(trapframe_t *tpf, int fd, void *buf, unsigned long count)
{
    if (curr_thread->file_descriptors_table[fd])
    {
        tpf->x0 = vfs_read(curr_thread->file_descriptors_table[fd], buf, count);
        return tpf->x0;
    }
    tpf->x0 = -1;
    return tpf->x0;
}

int mkdir(trapframe_t *tpf, const char *pathname, unsigned mode)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, pathname);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    tpf->x0 = vfs_mkdir(abs_path);
    return tpf->x0;
}

int mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, target);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);

    tpf->x0 = vfs_mount(abs_path, filesystem);
    return tpf->x0;
}

int chdir(trapframe_t *tpf, const char *path)
{
    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, path);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    strcpy(curr_thread->curr_working_dir, abs_path);

    return 0;
}

long lseek64(trapframe_t *tpf, int fd, long offset, int whence)
{
    if (whence == SEEK_SET) // used for dev_framebuffer
    {
        curr_thread->file_descriptors_table[fd]->f_pos = offset;
        tpf->x0 = offset;
    }
    else // other is not supported
    {
        tpf->x0 = -1;
    }

    return tpf->x0;
}
extern unsigned int height;
extern unsigned int isrgb;
extern unsigned int pitch;
extern unsigned int width;
int ioctl(trapframe_t *tpf, int fb, unsigned long request, void *info)
{
    if (request == 0) // used for get info (SPEC)
    {
        struct framebuffer_info *fb_info = info;
        fb_info->height = height;
        fb_info->isrgb = isrgb;
        fb_info->pitch = pitch;
        fb_info->width = width;
    }

    tpf->x0 = 0;
    return tpf->x0;
}