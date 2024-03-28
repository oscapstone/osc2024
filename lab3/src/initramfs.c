#include "initramfs.h"

char *CPIO_ADDR = 0;

int cpio_fdt_callback(char *nodename, char *propname, char *propvalue, unsigned int proplen){
    if(m_strcmp(propname, "linux,initrd-start\0") || m_strcmp(nodename, "chosen\0"))
        return -1;
    CPIO_ADDR = (char *)(long)read_bigendian((unsigned int *)propvalue); 
    return 0;
}

int cpio_get_start_addr(char **addr){
    if(CPIO_ADDR == 0)
        fdt_traverse(cpio_fdt_callback);
    *addr = CPIO_ADDR;
    
    return 0;
}

char* cpio_align_filename(char* addr){
    return addr + sizeof(cpio_newc_header);
}

char* cpio_align_filedata(char* filename, int namesize){
    return filename + 2 + ((namesize-2+3)/4)*4;
}

char* cpio_align_nextfile(char* filedata, int datasize){
    return filedata + ((datasize+3)/4)*4;
}

int cpio_parse(cpio_path* path){
    char* addr = path->next;
    if(addr == 0) return -1;
    
    cpio_newc_header* ptr = (cpio_newc_header *)addr;
    char magic[] = "070701\0";    
    for(int i=0; i<6; ++i)
        if(ptr->c_magic[i] != magic[i]) return -1;
    
    char c_namesize[9];
    char c_filesize[9];
    for(int i=0; i<8; ++i){
        c_namesize[i] = ptr->c_namesize[i];
        c_filesize[i] = ptr->c_filesize[i];
    }
    c_namesize[8] = '\0';
    c_filesize[8] = '\0';
    int namesize = m_htoi(c_namesize);
    int filesize = m_htoi(c_filesize);
    path->name = cpio_align_filename(addr);
    path->mode = ptr->c_mode[4];
    if(0 == m_strcmp(path->name, "TRAILER!!!\0"))
        return -1;
    path->file = cpio_align_filedata(path->name, namesize);
    path->next = cpio_align_nextfile(path->file, filesize);
    path->namesize = namesize;
    path->filesize = filesize;
    return 0;
}