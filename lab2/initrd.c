#include "uart.h"
#include "lib_func.h"
#include "definitions.h"

#define RAMDISK_START 0x20000000

/* Magic identifiers for the "cpio" file format. */
#define CPIO_HEADER_MAGIC "070701"
#define CPIO_FOOTER_MAGIC "TRAILER!!!"
#define CPIO_ALIGNMENT 4

/* cpio newc format */
struct cpio_newc_header
{
    char c_magic[6];     // "070701"
    char c_ino[8];       // inode number
    char c_mode[8];      // permissions
    char c_uid[8];       // user id
    char c_gid[8];       // group id
    char c_nlink[8];     // number of hard links
    char c_mtime[8];     // modification time
    char c_filesize[8];  // size of the file
    char c_devmajor[8];  // major number of the device
    char c_devminor[8];  // minor number of the device
    char c_rdevmajor[8]; // major number of the device for special files
    char c_rdevminor[8]; // minor number of the device for special files
    char c_namesize[8];  // length of the filename, including the final \0
    char c_check[8];     // checksum = 0
} __attribute__((packed));

/**
 * List the contents of an archive
 */
void initrd_list()
{
    char *buf = (char *)RAMDISK_START;
    uart_puts("Type       Offset   Size     Filename\n");

    while (!memcmp(buf, CPIO_HEADER_MAGIC, 6))
    {
        if (!memcmp(buf + sizeof(struct cpio_newc_header), CPIO_FOOTER_MAGIC, 10)) {
            // uart_puts("TRAILER!!!\n");
            break;
        }
        struct cpio_newc_header *header = (struct cpio_newc_header *)buf;
        int ns = hex2bin(header->c_namesize, sizeof(header->c_namesize));
        int fs = hex2bin(header->c_filesize, sizeof(header->c_filesize));

        // Check if the entry is a directory
        int mode = hex2bin(header->c_mode, sizeof(header->c_mode));
        if ((mode & 0170000) == 0040000) {
            uart_puts("Directory ");
        } else {
            uart_puts("File      ");
        }

        // Print out meta information
        //uart_hex(mode); // mode (access rights + type)
        uart_send(' ');
        uart_hex((unsigned int)((unsigned long)buf) + sizeof(struct cpio_newc_header) + ns);
        uart_send(' ');
        uart_hex(fs); // file size in hex
        uart_send(' ');
        // uart_hex(hex2bin(header->c_uid, 8)); // user id in hex
        // uart_send('.');
        // uart_hex(hex2bin(header->c_gid, 8)); // group id in hex
        // uart_send('\t');
        uart_puts(buf + sizeof(struct cpio_newc_header)); // filename
        uart_puts("\n");

        // Jump to the next file
        buf += ((sizeof(struct cpio_newc_header) + ns + ((fs + 3) & ~3)) + 3) & ~3;
    }
}

// void initrd_currentdir_list(const char *current_dir)
// {
//     char *buf = (char *)RAMDISK_START;
//     uart_puts("Type     Offset   Size     Access rights\tFilename\n");

//     while (!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(struct cpio_newc_header), "TRAILER!!", 9))
//     {
//         struct cpio_newc_header *header = (struct cpio_newc_header *)buf;
//         int ns = hex2bin(header->c_namesize, 8);
//         int fs = hex2bin(header->c_filesize, 8);
//         char *filename = buf + sizeof(struct cpio_newc_header);

//         // Check if the entry is in the current directory.
//         if (strncmp(filename, current_dir, strlen(current_dir)) == 0)
//         {
//             char *relative_filename = filename + strlen(current_dir);
//             // Check if the entry is not in a subdirectory.
//             if (strchr(relative_filename, '/') == NULL || (relative_filename[strlen(relative_filename) - 1] == '/' && strchr(relative_filename, '/') == strrchr(relative_filename, '/')))
//             {
//                 // Print out meta information.
//                 uart_hex(hex2bin(header->c_mode, 8)); // mode (access rights + type)
//                 uart_send(' ');
//                 uart_hex((unsigned int)((unsigned long)buf) + sizeof(struct cpio_newc_header) + ns);
//                 uart_send(' ');
//                 uart_hex(fs); // file size in hex
//                 uart_send(' ');
//                 uart_hex(hex2bin(header->c_uid, 8)); // user id in hex
//                 uart_send('.');
//                 uart_hex(hex2bin(header->c_gid, 8)); // group id in hex
//                 uart_send('\t');
//                 uart_puts(relative_filename); // filename
//                 uart_puts("\n");
//             }
//         }

//         // Jump to the next file.
//         buf += ((sizeof(struct cpio_newc_header) + ns + ((fs + 3) & ~3)) + 3) & ~3;
//     }
// }

void initrd_cat(char *filename)
{
    char *buf = (char *)RAMDISK_START;
    // If it's a cpio archive. Cpio also has a trailer entry.
    while (!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(struct cpio_newc_header), "TRAILER!!", 9))
    {
        struct cpio_newc_header *header = (struct cpio_newc_header *)buf;
        int ns = hex2bin(header->c_namesize, 8);
        int fs = hex2bin(header->c_filesize, 8);
        char *file_name = buf + sizeof(struct cpio_newc_header);

        // Check if the entry is the file we're looking for.
        if (strcmp(file_name, filename) == 0)
        {
            // Check if it's a directory.
            if ((hex2bin(header->c_mode, 8) & 0170000) == 0040000)
            {
                uart_puts("This is a directory\n");
                return; // Exit the function.
            }
            // Print the file content.
            char *file_content = buf + sizeof(struct cpio_newc_header) + ns;
            for (int i = 0; i < fs; i++)
            {
                uart_send(file_content[i]);
            }
            uart_puts("\n");
            return; // Exit the function after printing the file content.
        }

        // Jump to the next file.
        buf += ((sizeof(struct cpio_newc_header) + ns + ((fs + 3) & ~3)) + 3) & ~3;
    }

    // File not found.
    uart_puts("File not found\n");
}