#ifndef _INITRAMDISK_H_
#define _INITRAMDISK_H_

#define MAX_HEADERS 10
#define MAX_DIRECTORIES 10
#define MAX_FILES 10
#define MAX_NAME_LENGTH 64
#define MAX_FILE_SIZE 512

struct CpioHeader {
    unsigned int inode;      // inode number
    unsigned int mode;       // File mode
    unsigned int uid;        // User ID
    unsigned int guid;       // Group ID
    unsigned int nlink;      // Number of links
    unsigned int mtime;      // Modification time
    unsigned int fileSize;   // Size of file
    unsigned int devMaj;     // Major number of device
    unsigned int devMin;     // Minor number of device
    unsigned int rdevMaj;    // Major number of a character or block device file
    unsigned int rdevMin;    // Minor number of a character or block device file
    unsigned int nameSize;   // Length of name including the trailing NUL
    unsigned int check;      // Checksum
};

struct File {
    struct CpioHeader* fHeader;

    // Maximum size for a file name is 64 bytes.
    char fName[64];

    // Maximum Data for a file name is 512 bytes.
    char fData[MAX_FILE_SIZE];

    unsigned int fileSize;

    // Stores the parent directory.
    struct Directory* parent;
};

struct Directory {
    struct CpioHeader* dHeader;

    // Maximum size for a directory name is 64 bytes.
    char dName[64];

    // Keep track of the number of files inside that directory.
    int nFiles;
    // Keep track of the number of directories inside that directory.
    int nDirs;
    // Keep track of current empty directory index.
    int emptyDirInd;
    // Keep track of current empty file index.
    int emptyFileInd;

    // Stores the parent directory.
    struct Directory* parent;
    // Define the maximum number of directories and files stored within a directory.
    struct Directory* dirs[MAX_DIRECTORIES];
    struct File* files[10];
};

// Static memory pool for headers.
static struct CpioHeader headerPool[MAX_HEADERS];
static int headerPoolIndex = 0;

// Static memory pool for directories.
static struct Directory directoryPool[MAX_DIRECTORIES];
static int directoryPoolIndex = 0;

// Static memory pool for files.
static struct File filePool[MAX_FILES];
static int filePoolIndex = 0;

extern struct Directory* rootDir;
extern struct Directory* userDir;

void initramdisk_main();
struct CpioHeader* parse_cpio_header();
struct Directory* allocateDirectory();
struct File* allocateFile();
struct CpioHeader* allocateHeader();
void show_current_dir_content();
void change_directory(char* path);
void print_content(char* arg);

#endif