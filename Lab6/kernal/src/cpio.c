#include "stdio.h"
#include"cpio.h"
#include"str.h"
#include"memalloc.h"
void* cpio_addr;

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

void cpio_ls()
{
    char* pointer=cpio_addr;
    puts("cpio_addr:0x");
    put_hex((unsigned long long)cpio_addr);
    puts("\r\n");
    // struct cpio_newc_header* head=(struct cpio_newc_header*)pointer;
    // //check magic value
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
        puts("\r\nfiledize:");
        put_int(filesize);
        puts("\r\n");
        pointer+=sizeof(struct cpio_newc_header);
        //pointer=mem_alin(pointer,4);
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
        //ptr=mem_alin(ptr,4);
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

int cpio_load(char* filename,char* load_base){
    puts("want to load filename:");
    puts(filename);
    puts("\r\n");
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
        //ptr=mem_alin(ptr,4);
        ptr=mem_alin(ptr+namesize,4);
        ptr=mem_alin(ptr+filesize,4);
    }
    if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),"TRAILER!!!")){
        puts("no such file.\r\n");
        return 1;
    }
    else if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),filename)){
        //output file content
        struct cpio_newc_header* head=(struct cpio_newc_header*)ptr;
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        unsigned int filesize=hex_to_int(head->c_filesize,8);
        ptr+=(sizeof(struct cpio_newc_header))+namesize;
        ptr=mem_alin(ptr,4);
        // puts(ptr);
        // puts("\r\n");

        for(int i=0;i<filesize;i++){
            *load_base=*ptr;
            load_base++;
            ptr++;
        }
        *load_base='\0';
        puts("load successful\r\n");
        return 0;
    }
    else{
        puts("loadimg error\r\n");
        return 1;
    }
}

unsigned int cpio_size(char* filename){
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
        //ptr=mem_alin(ptr,4);
        ptr=mem_alin(ptr+namesize,4);
        ptr=mem_alin(ptr+filesize,4);
    }
    if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),"TRAILER!!!")){
        puts("no such file.\r\n");
        return 0;
    }
    else if(!strcmp((char*)ptr+sizeof(struct cpio_newc_header),filename)){
        //output file content
        struct cpio_newc_header* head=(struct cpio_newc_header*)ptr;
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        unsigned int filesize=hex_to_int(head->c_filesize,8);
        ptr+=(sizeof(struct cpio_newc_header))+namesize;
        ptr=mem_alin(ptr,4);
        // puts(ptr);
        // puts("\r\n");
        return filesize;
    }
    else{
        puts("loadimg error\r\n");
        return 0;
    }
}

unsigned long long cpio_end()
{
    char* pointer=cpio_addr;
    puts("pinter:");
    put_long_hex(pointer);
    puts("\r\ncpio_addr:");
    put_long_hex((unsigned long long)cpio_addr);
    puts("\r\n");
    int flag=0;
    while(1){
        if(!strcmp((char*)pointer+sizeof(struct cpio_newc_header),"TRAILER!!!")){
            flag=1;
        }
        struct cpio_newc_header* head=(struct cpio_newc_header*)pointer;
        if(strcmp_len((char*)head,"070701",6)){
            puts("error: cpio magic value error\r\n");
            puts_len((char*)pointer,6);
            puts("\r\n");
            return 0;
        }
        unsigned int namesize=hex_to_int(head->c_namesize,8);
        unsigned int filesize=hex_to_int(head->c_filesize,8);
        pointer+=sizeof(struct cpio_newc_header);
        //pointer=mem_alin(pointer,4);
        pointer=mem_alin(pointer+namesize,4);
        pointer=mem_alin(pointer+filesize,4);
        if(flag)break;
    }
    puts("pinter before return:");
    put_long_hex(pointer);
    puts("\r\n");
    return pointer;
}