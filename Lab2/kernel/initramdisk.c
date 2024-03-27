#include "initramdisk.h"
#include "../peripherals/mini_uart.h"
#include "../peripherals/utils.h"
#include <stdint.h>

struct Directory* rootDir;
// Save the current user location.
struct Directory* userDir;

// Points to the cpio archive. The beginning address is received from dtb.
char* cpio;

// Read the cpio file and construct the simple filesystem.
void initramdisk_main(uintptr_t cpio_addr) {
    // Retrieve the cpio address from parsing the device tree blob.
    cpio = (char *)cpio_addr;
    
    // Parse info for root directory.
    rootDir = allocateDirectory();
    rootDir->dHeader = parse_cpio_header();
    rootDir->nDirs = 0;
    rootDir->nFiles = 0;
    rootDir->emptyDirInd = 0;
    rootDir->emptyFileInd = 0;
    // Root directory has no parent.
    rootDir->parent = NULL;
    userDir = rootDir;

    for (int i = 0; i < rootDir->dHeader->nameSize; i++, cpio++) {
        rootDir->dName[i] = *cpio;
    }

    // Align to 4-byte boundary.
    if (((unsigned long)cpio & 3)) {
        unsigned long pad = 4 - ((unsigned long)cpio & 3);
        cpio += pad;
    }

    // Common pattern for directories is '4' at the beginning of the 'mode' value in octal
    // representation(040xxx).
    // Parse the entire cpio.
    while (1) {
        struct Directory* curDir = rootDir;
        struct CpioHeader* header = parse_cpio_header();

        char end[15];
        for (int i = 0; i < 11; i++) {
            end[i] = *(cpio + i);
        }

        if (strcmp(end, "TRAILER!!!") == 0) {
            break;
        }

        // New directory path.
        char path[64];
        // Records number of nested directories storing the file or directory.
        int nested_dir_num = 0;

        // The null character is included in the nameSize.
        for (int i = 0; i < header->nameSize; i++, cpio++) {
            path[i] = *cpio;
            if (path[i] == '/')
                nested_dir_num++;
        }

        // Align to 4-byte boundary.
        if (((unsigned long)cpio & 3)) {
            unsigned long pad = 4 - ((unsigned long)cpio & 3);
            cpio += pad;
        }

        // Navigate to the directory the new directory or new file is located.
        int pathInd = 0;

        // Enter if not located in root directory.
        while (nested_dir_num > 0) {

            char nextDir[64];

            int i = 0;
            for (; path[pathInd] != '/'; i++, pathInd++) {
                nextDir[i] = path[pathInd];
            }
            nextDir[i] = '\0';
            pathInd++;

            for (int i = 0; i < curDir->nDirs; i++) {
                // Check which directory to navigate to.
                if (strcmp(nextDir, curDir->dirs[i]->dName) == 0) {
                    curDir = curDir->dirs[i];
                    nested_dir_num--;
                    break;
                }
            }
        }

        char name[64];
        // Read the new directory(file) name.
        for (int i = 0; pathInd < header->nameSize; i++, pathInd++) {
            name[i] = path[pathInd];
        }

        // New directory.
        if (header->mode & 040000) {
            // Initialize new directory.
            struct Directory* newDir = allocateDirectory();
            newDir->dHeader = header;
            newDir->emptyDirInd = 0;
            newDir->emptyFileInd = 0;
            newDir->nDirs = 0;
            newDir->nFiles = 0;
            newDir->parent = curDir;
            strcpy(newDir->dName, name);

            // Link the new directory with its parent directory.
            curDir->nDirs++;
            curDir->dirs[curDir->emptyDirInd++] = newDir;
            
        // New file.
        } else if (header->mode & 0100000) {
            // Initialize new file.
            struct File* newFile = allocateFile();
            newFile->fileSize = header->fileSize;
            newFile->fHeader = header;
            newFile->parent = curDir;
            strcpy(newFile->fName, name);

            for (int i = 0; i < header->fileSize; i++, cpio++) {
                newFile->fData[i] = *cpio;
            }

            // Link the new file with its parent directory.
            curDir->nFiles++;
            curDir->files[curDir->emptyFileInd++] = newFile;

            // Align to 4-byte boundary.
            if (((unsigned long)cpio & 3)) {
                unsigned long pad = 4 - ((unsigned long)cpio & 3);
                cpio += pad;
            }
        }
    }
}

struct CpioHeader* parse_cpio_header() {

    struct CpioHeader* header = allocateHeader();
    char tmp[10];
    // Record the length of tmp.
    int len = 8;

    // Read magic number from cpio.
    for (int i = 0; i < 6; i++) {
        tmp[i] = *cpio++;
    }
    tmp[6] = '\0';

    if (strcmp(tmp, "070701") != 0) {
        uart_send_string("Cpio not in new Ascii format!\r\n");
    }

    // Read inode number from cpio.
    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->inode = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->mode = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->uid = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->guid = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->nlink = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->mtime = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->fileSize = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->devMaj = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->devMin = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->rdevMaj = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->rdevMin = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->nameSize = strHex2Int(tmp, len);

    for (int i = 0; i < 8; i++) {
        tmp[i] = *cpio++;
    }
    header->check = strHex2Int(tmp, len);

    return header;
}

struct File* allocateFile() {
    if (filePoolIndex < MAX_FILES)
        return &filePool[filePoolIndex++];
    else {
        uart_send_string("Maximum file count reached! Failed to create new file!\r\n");
        return NULL;
    }
}

struct Directory* allocateDirectory() {
    if (directoryPoolIndex < MAX_DIRECTORIES)
        return &directoryPool[directoryPoolIndex++];
    else {
        uart_send_string("Maximum directory count reached! Failed to create new directory!\r\n");
        return NULL;
    }
}

struct CpioHeader* allocateHeader() {
    if (headerPoolIndex < MAX_HEADERS)
        return &headerPool[headerPoolIndex++];
    else {
        uart_send_string("Maximum header count reached! Failed to create new header!\r\n");
        return NULL;
    }
}


void show_current_dir_content() {
    for (int i = 0; i < userDir->nDirs; i++) {
        uart_send_string(userDir->dirs[i]->dName);
        uart_send_string("\r\n");
    }

    for (int i = 0; i < userDir->nFiles; i++) {
        uart_send_string(userDir->files[i]->fName);
        uart_send_string("\r\n");
    }
}

void change_directory(char* path) {
    for (int i = 0; i < userDir->nDirs; i++) {
        if (strcmp(path, userDir->dirs[i]->dName) == 0) {
            userDir = userDir->dirs[i];
            return;
        }
    }

    uart_send_string("No such directory!\r\n");
}

void print_content(char* fileName) {
    for (int i = 0; i < userDir->emptyFileInd; i++) {
        if (strcmp(fileName, userDir->files[i]->fName) == 0) {
            for (unsigned int j = 0; j < userDir->files[i]->fileSize; j++) {
                if (userDir->files[i]->fData[j] == '\n') {
                    uart_send_string("\r\n");
                } else {
                    uart_send(userDir->files[i]->fData[j]);
                }
            }
            uart_send_string("\r\n");
            return;
        }
    }

    uart_send_string("File doesn't exist!\r\n");
}

