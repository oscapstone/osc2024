#ifndef KERNEL_H
#define KERNEL_H

/* If DEBUG_MULTITASKING is defined, some logs in kernel for multi-tasking will be printed out. */
// #define DEBUG_MULTITASKING
/* If DEBUG_SYSCALL is defined, some logs in kernel for system call will be printed out. */
//#define DEBUG_SYSCALL

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		     5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

#define ALIGN(x, a)       (((x) + (a) - 1) & ~(a - 1)) // align up. e.g. ALIGN(6,8) = 8, ALIGN(9, 8) = 8.

#define min(a, b) ((a) <= (b) ? (a) : (b))
#define max(a, b) ((a) >= (b) ? (a) : (b))

#define clamp(val, lo, hi) (min((max(val, lo)), (hi)))

/* memmove: move 'n' byte from 'src' to 'dest'. */
static inline void memmove(void *dest, const void *src, unsigned int n) {
    char *csrc = (char *)src;
    char *cdest = (char *)dest;
    if (src < dest)
        for (int i = n - 1; i >= 0; i--)
            cdest[i] = csrc[i];
    else
        for (int i = 0; i < n; i++)
            cdest[i] = csrc[i];
}

#endif