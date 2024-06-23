#include "user_demo.h"
#include "fork.h"
#include "schedule.h"
#include "io.h"
#include "sys.h"

// system call
extern int getpid();
extern int fork();
extern void exit();
extern int open(const char *pathname, int flags);
extern int close(int fd);
extern int read(int fd, void *buf, unsigned long count);
extern int write(int fd, const void *buf, unsigned long count);

static void user_multiple_thread_test_foo();
static void user_fork_test_foo();
static void user_open_test_foo();
static void user_open_test_initramfs_foo();
static void user_read_test_foo();
static void user_write_test_foo();
static void user_write_test_initramfs_foo();
static void user_stdout_test_foo();
static void user_stdin_test_foo();

// lab5
void multiple_thread_test(int argc, char *argv[])
{
    for(int i = 0; i < 5; ++i) {
        copy_process(PF_KTHREAD, (unsigned long)&user_multiple_thread_test_foo, 0, 0);
    }
}

void user_fork_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_fork_test_foo, 0);
}

// lab7

void user_open_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_open_test_foo, 0);
}

void user_open_test_initramfs(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_open_test_initramfs_foo, 0);
}

void user_read_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_read_test_foo, 0);
}

void user_write_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_write_test_foo, 0);
}

void user_write_test_initramfs(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_write_test_initramfs_foo, 0);
}

void user_stdout_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_stdout_test_foo, 0);
}

void user_stdin_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_stdin_test_foo, 0);
}

// ------------------------------------------------------------------

static void user_multiple_thread_test_foo()
{
    for(int i = 0; i < 10; ++i) {
        printf("\r\nThread id: "); printf_int(current->pid); printf("\t,loop: "); printf_int(i);
        delay(1000000);
        schedule();
    }
    current->state = TASK_ZOMBIE;
    while(1);
}

static void user_fork_test_foo()
{
    // printf("Fork Test , pid : %d\n",getpid());
    printf("\r\nFork Test, pid: "); printf_int(getpid());
    uint32_t cnt = 1,ret=0;

    if((ret=fork()) == 0){ //pid == 0 => child
        printf("\r\n===== Child Process =====");
        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        // printf("first  child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        printf("\r\nfirst child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
        printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
        ++cnt;

        if ((ret = fork() )!= 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            // printf("first  child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
            printf("\r\nfirst child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
            printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                // printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                printf("\r\nsecond child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
                printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
                delay(1000000);
                ++cnt;
            }
        }
        exit();
    }
    else{ //pid > 0 => parent
        printf("\r\n===== Parent Process =====");
        printf("\r\nParent Process, pid: "); printf_int(getpid());
        printf(" child pid: "); printf_int(ret);
        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf(" cnt: "); printf_int(cnt); printf(" , ptr: "); printf_hex((unsigned long)&cnt);
        printf(" , sp: "); printf_hex(cur_sp);
        exit();
    }
}

static void user_open_test_foo()
{
    printf("\r\nUser Open Test pid: "); printf_int(getpid());
    // char buff[] = "Hello, World!";
    int fd = open("test.txt", O_CREAT);
    if(fd == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd);
    // write(fd, buff, sizeof(buff));
    close(fd); 
    exit();
}

static void user_open_test_initramfs_foo()
{
    printf("\r\nUser Open Test Initramfs pid: "); printf_int(getpid());
    // char buff[] = "Hello, World!";
    int fd = open("/initramfs/test.txt", O_CREAT);
    if(fd == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd);
    // write(fd, buff, sizeof(buff));
    close(fd); 
    exit();
}

static void user_read_test_foo()
{
    printf("\r\nUser Read Test pid: "); printf_int(getpid());
    char buff1[100];
    int fd1 = open("/initramfs/f1", 0);
    if(fd1 == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd1);
    read(fd1, buff1, sizeof(buff1));
    printf("\r\nRead content f1: "); printf(buff1);

    char buff2[4];
    int fd2 = open("/initramfs/f2", 0);
    if(fd2 == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd2);
    read(fd2, buff2, sizeof(buff2));
    printf("\r\nRead content f2: "); printf(buff2);
    read(fd2, buff2, sizeof(buff2));
    printf("\r\nRead content f2: "); printf(buff2);
    close(fd1);
    close(fd2);
    exit();
}

static void user_write_test_foo()
{
    printf("\r\nUser Write Test pid: "); printf_int(getpid());
    char buff[] = "Hello, World!";
    int fd = open("/write.txt", O_CREAT);
    if(fd == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd);
    write(fd, buff, sizeof(buff));
    close(fd); 
    
    int fd2 = open("/write.txt", 0);
    if(fd2 == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd2);
    char buff2[100];
    read(fd2, buff2, sizeof(buff2));
    printf("\r\nRead content write.txt: "); printf(buff2);
    close(fd2);
    exit();
}

static void user_write_test_initramfs_foo()
{
    printf("\r\nUser Write Test Initramfs pid: "); printf_int(getpid());
    char buff[] = "Hello, World!";
    int fd = open("/initramfs/f1", 0);
    if(fd == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd);
    int size = write(fd, buff, sizeof(buff));
    if(size < 0) {
        printf("\r\nWrite file failed!");
        exit();
    }
    close(fd); 

    int fd2 = open("/initramfs/f1", 0);
    if(fd2 == -1){
        printf("\r\nOpen file failed!");
        exit();
    }
    printf("\r\nOpen file success, fd: "); printf_int(fd2);
    char buff2[100];
    read(fd2, buff2, sizeof(buff2));
    printf("\r\nRead content f1: "); printf(buff2);
    close(fd2);
    exit();
}

static void user_stdout_test_foo()
{
    write(1, "\nhello\n", 6);
    exit();
}

static void user_stdin_test_foo()
{
    char buf[100];
    read(0, buf, 20);
    write(1, buf, 20);
    exit();
}