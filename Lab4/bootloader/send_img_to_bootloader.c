#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

int set_interface_attribs(int fd, int speed) {
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        perror("Error from tcgetattr");
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    //tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    int fd;
    struct stat st;
    char *device = "/dev/ttyUSB0";
    int baud = 115200;
    char *filename = "../kernal/kernel8.img";
    unsigned char *buffer;
    int file_fd;
    off_t length, i;

    if (argc > 1) filename = argv[1];
    if (argc > 2) device = argv[2];
    if (argc > 3) baud = atoi(argv[3]);

    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("Failed to open file");
        return -1;
    }

    if (fstat(file_fd, &st) == -1) {
        perror("Failed to get file size");
        close(file_fd);
        return -1;
    }
    length = st.st_size;

    buffer = malloc(length);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        close(file_fd);
        return -1;
    }
    read(file_fd, buffer, length);
    close(file_fd);

    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Error opening serial port");
        free(buffer);
        return -1;
    }

    set_interface_attribs(fd, baud);

    printf("Kernel image size: %lx\n", length);
    unsigned char read_back;
    for(i=0;i<8;i++){
        unsigned char byte = (length >> (8 * i)) & 0xFF;
        write(fd, &byte, 1);
        read(fd, &read_back, 1);

        if (read_back != byte) {
            printf("Error: transmiting img size\n");
            break; // Optionally stop on error
        }
    }
    for (i = 0; i < length; ++i) {
        write(fd, &buffer[i], 1);  // Send a single byte
        read(fd, &read_back, 1);   // Read back the byte
        
        if (read_back != buffer[i]) {
            printf("Error: Byte mismatch at %lu, sent 0x%x, received 0x%x\n", i, buffer[i], read_back);
            break; // Optionally stop on error
        }

        if (i % 100 == 0 || read_back != buffer[i]) {
            printf("%lu/%lu bytes\n", i, length);
        }
    }

    printf("%lu/%lu bytes\n", length, length);
    printf("Transfer finished!\n");
    free(buffer);
    close(fd);

    return 0;
}
