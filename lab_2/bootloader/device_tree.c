#include "include/device_tree.h"
#include <stdio.h>
#include <stdlib.h>


void header_parser(){

}

#include <stdio.h>
#include <stdlib.h>

unsigned long* mock_load_file() {
    FILE *file = fopen("bcm2710-rpi-3-b-plus.dtb", "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Allocate memory to hold the entire file
    unsigned char *buffer = (unsigned char *)malloc(fileSize);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    // Read the entire file into the buffer
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize) {
        perror("Failed to read file");
        free(buffer);
        fclose(file);
        return 1;
    }

    return (unsigned long*)buffer;
}
device_tree_header myHeader;

int main(){
    uint32_t* start;
    uint32_t* dtb = mock_load_file();
    start = dtb;
    // __builtin_bswap32() is a GCC built-in function to convert to the system's endianness.(RPi uses little-endian)
    myHeader.magic = __builtin_bswap32(*dtb++);
    if ((myHeader.magic - 0xd00dfeed) != 0) {
        printf("DTB magic number doesn't match!\r\n");
        return 1;
    }
    myHeader.totalsize = __builtin_bswap32(*dtb++);
    myHeader.off_dt_struct = __builtin_bswap32(*dtb++);
    myHeader.off_dt_strings = __builtin_bswap32(*dtb++);
    myHeader.off_mem_rsvmap = __builtin_bswap32(*dtb++);
    myHeader.version = __builtin_bswap32(*dtb++);
    myHeader.last_comp_version = __builtin_bswap32(*dtb++);
    myHeader.boot_cpuid_phys = __builtin_bswap32(*dtb++);
    myHeader.size_dt_strings = __builtin_bswap32(*dtb++);
    myHeader.size_dt_struct = __builtin_bswap32(*dtb++);
    printf("%lu\n", myHeader.off_dt_struct);
    printf("%lu\n", myHeader.totalsize);

    // jump to structure
    uint32_t* structure_ptr = (uint32_t *)(start + myHeader.off_dt_struct);
    while(1){
        uint32_t token = __builtin_bswap32(*structure_ptr++);
        if(token == FDT_BEGIN_NODE){
            printf("begin");
        }
    }

    //The node’s name as a null-terminated string



}
