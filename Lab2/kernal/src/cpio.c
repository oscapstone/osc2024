#include "stdio.h"
#include"cpio.h"
#include"str.h"
char* cpio_addr;

unsigned int hex_to_int(char* arr,int len){
    if(!len) return 0;

    unsigned int value=0;
    if(*arr <= '9' && *arr >='0'){
        value=(unsigned int)(*arr-'0')<<((len-1)*4);
    }
    else if(*arr <= 'f' && *arr >= 'a'){
        value=(unsigned int)(*arr-'a'+10)<<((len-1)*4);
    }
    else if(*arr <= 'F' && *arr >= 'A'){
        value=(unsigned int)(*arr-'A'+10)<<((len-1)*4);
    }
    else{
        puts("error:hex_to_int\r\n");
    }
    return value+hex_to_int(arr+1,len-1);
}

char* mem_alin(char* ptr,int alin){
    if((unsigned long long)ptr%alin)ptr+=(alin-(unsigned long long)ptr%alin);
    return ptr;
}

void cpio_ls()
{
    char* pointer=cpio_addr;
    puts("apio_addr:0x");
    put_hex((unsigned long long)cpio_addr);
    puts("\r\n");
    //struct cpio_newc_header* head=(struct cpio_newc_header*)pointer;
    //check magic value
    // if(strcmp_len(pointer,"070701",6)){
    //     puts("error: cpio magic value error\r\n");
    //     puts_len((char*)pointer,6);
    //     puts("\r\n");
    //     return;
    // }
    // else{
    //     puts("magic:0x");
    //     puts_len((char*)pointer,6);
    //     puts("\r\n");
    //     puts("hex_to_int:0x");
    //     put_hex(hex_to_int((char*)pointer,6));
    //     puts("\r\n");

    // }
    while(strcmp((char*)pointer+sizeof(struct cpio_newc_header),"TRAILER!!!")){
        struct cpio_newc_header* head=(struct cpio_newc_header*)pointer;
        if(strcmp_len((char*)head,"070701",6)){
            puts("error: cpio magic value error\r\n");
            puts_len((char*)pointer,6);
            puts("\r\n");
            break;
        }
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        unsigned int filesize=hex_to_int(head->c_filesize,8);
        puts_len(pointer+sizeof(struct cpio_newc_header),namesize);
        puts("\r\n");
        pointer+=sizeof(struct cpio_newc_header);
        pointer=mem_alin(pointer+namesize,4);
        pointer=mem_alin(pointer+filesize,4);
    }
    return;
}
 
void cpio_cat(char* filename){
    char* ptr=cpio_addr;
    while(strcmp((char*)ptr+sizeof(struct cpio_newc_header),"TRAILER!!!") && strcmp((char*)ptr+sizeof(struct cpio_newc_header),filename)){
        struct cpio_newc_header* head=(struct cpio_newc_header*)ptr;
        if(strcmp_len((char*)head,"070701",6)){
            puts("error: cpio magic value error\r\n");
            puts_len((char*)ptr,6);
            puts("\r\n");
            break;
        }
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        unsigned int filesize=hex_to_int(head->c_filesize,8);
        ptr+=sizeof(struct cpio_newc_header);
        ptr=mem_alin(ptr+namesize,4);
        ptr=mem_alin(ptr+filesize,4);
    }
    if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),"TRAILER!!!")){
        puts("no such file.\r\n");
    }
    else if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),filename)){
        //output file content
        struct cpio_newc_header* head=(struct cpio_newc_header*)ptr;
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        ptr+=(sizeof(struct cpio_newc_header))+namesize;
        ptr=mem_alin(ptr,4);
        puts(ptr);
        puts("\r\n");
    }
    else{
        puts("cat error\r\n");
    }
}