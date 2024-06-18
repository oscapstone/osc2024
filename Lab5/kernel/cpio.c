#include "cpio.h"

uint32_t DEVTREE_CPIO_BASE = 0;
uint32_t DEVTREE_CPIO_END = 0;
extern thread_t* cur_thread;

void cpio_ls(){
    struct cpio_newc_header *head = (struct cpio_newc_header *)DEVTREE_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header);

    while (strncmp(head->c_magic, CPIO_MAGIC, MAGIC_SIZE)){

        char* f_name = ((char*) head) + h_size;

        uint32_t name_len = atoi(head->c_namesize, FIELD_SIZE);
        uint32_t f_size = atoi(head->c_filesize, FIELD_SIZE);
        uint32_t file_offset = h_size + name_len;

        if (strncmp(f_name, CPIO_END, name_len))
            break;

        print_newline();
        print_nchar(f_name, name_len);
        
        if (file_offset % 4 != 0) 
            file_offset = 4 * ((file_offset + 4) / 4);

        if(f_size % 4 != 0) 
            f_size = 4 * ((f_size + 4) / 4);

        head = (struct cpio_newc_header*)((uint8_t*)head + file_offset + f_size);
    }
}

void cpio_cat(){
    struct cpio_newc_header *head = (struct cpio_newc_header *)DEVTREE_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header);

    char input_buffer[256];

    print_str("\nFilename: ");
    read_input(input_buffer);

    int no_match = 1;

    while (strncmp(head->c_magic, CPIO_MAGIC, MAGIC_SIZE)){
        char* f_name = ((char*)head) + h_size;

        uint32_t name_len = atoi(head->c_namesize, FIELD_SIZE);
        uint32_t f_size = atoi(head->c_filesize, FIELD_SIZE);
        uint32_t file_offset = h_size + name_len;

        if (strncmp(f_name, CPIO_END, name_len))
            break;

        if (file_offset % 4 != 0) 
            file_offset = 4 * ((file_offset + 4) / 4);

        if (strncmp(f_name, input_buffer, name_len)){
            no_match = 0;
            char* file_content = (char*)head + file_offset;

            print_newline();

            for (int i = 0; i < f_size; i++){
                print_char(file_content[i]);
            }

            break;
        }

        if(f_size % 4 != 0) 
            f_size = 4 * ((f_size + 4) / 4);

        head = (struct cpio_newc_header*)((uint8_t*)head + file_offset + f_size);
    }

    if (no_match)
        print_str("\nNo match file");
}

void cpio_exec(){
    struct cpio_newc_header *head = (struct cpio_newc_header *)DEVTREE_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header);

    char input_buffer[256];
    print_str("\nFile to be executed: ");
    read_input(input_buffer);

    int no_match = 1;

    while (strncmp(head->c_magic, CPIO_MAGIC, MAGIC_SIZE)){
        char* f_name = ((char*)head) + h_size;

        uint32_t name_len = atoi(head->c_namesize, FIELD_SIZE);
        uint32_t f_size = atoi(head->c_filesize, FIELD_SIZE);
        uint32_t file_offset = h_size + name_len;

        if (strncmp(f_name, CPIO_END, name_len))
            break;

        if (file_offset % 4 != 0) 
            file_offset = 4 * ((file_offset + 4) / 4);

        if (strncmp(f_name, input_buffer, name_len)){
            no_match = 0;
            char* file_content = (char*)head + file_offset;
            // print_hex((uint32_t)file_content);

            // uint32_t sp_loc = 0x3600000;

            // print_str("\nhere");

            // new_user_thread(file_content, f_size);
            thread_exec(file_content, f_size);

            break;
        }

        if(f_size % 4 != 0) 
            f_size = 4 * ((f_size + 4) / 4);

        head = (struct cpio_newc_header*)((uint8_t*)head + file_offset + f_size);
    }

    if (no_match)
        print_str("\nNo match file");
}

void initramfs_callback(char* node_name, char* prop_name, struct fdt_prop* prop){

    if (strncmp(node_name, "chosen", 7) && 
        strncmp(prop_name, "linux,initrd-start", 19)) {

        DEVTREE_CPIO_BASE = to_little_endian(*((uint32_t*)(prop + 1)));
        // print_str("\nDevice Tree Base: 0x");
        // print_hex(DEVTREE_CPIO_BASE);

    }else if (strncmp(node_name, "chosen", 7) &&
        strncmp(prop_name, "linux,initrd-end", 17)){
        DEVTREE_CPIO_END = to_little_endian(*((uint32_t*)(prop + 1)));
        // print_str("\nDevice Tree End: 0x");
        // print_hex(DEVTREE_CPIO_END);
    }
}

char* get_cpio_file(char* name){

    struct cpio_newc_header *head = (struct cpio_newc_header *)DEVTREE_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header);

    while (strncmp(head->c_magic, CPIO_MAGIC, MAGIC_SIZE)){
        char* f_name = ((char*)head) + h_size;

        uint32_t name_len = atoi(head->c_namesize, FIELD_SIZE);
        uint32_t f_size = atoi(head->c_filesize, FIELD_SIZE);
        uint32_t file_offset = h_size + name_len;

        if (strncmp(f_name, CPIO_END, name_len))
            break;

        if (file_offset % 4 != 0) 
            file_offset = 4 * ((file_offset + 4) / 4);

        if (strncmp(f_name, name, name_len)){
            char* file_content = (char*)head + file_offset;
            // print_hex((uint32_t)file_content);

            // print_str("\nhere");
            return file_content;
        }

        if(f_size % 4 != 0) 
            f_size = 4 * ((f_size + 4) / 4);

        head = (struct cpio_newc_header*)((uint8_t*)head + file_offset + f_size);
    }

    print_str("\nNo Match File");

    return 0;
}

int get_cpio_fsize(char* name){
    struct cpio_newc_header *head = (struct cpio_newc_header *)DEVTREE_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header);

    while (strncmp(head->c_magic, CPIO_MAGIC, MAGIC_SIZE)){
        char* f_name = ((char*)head) + h_size;

        uint32_t name_len = atoi(head->c_namesize, FIELD_SIZE);
        uint32_t f_size = atoi(head->c_filesize, FIELD_SIZE);
        uint32_t file_offset = h_size + name_len;

        if (strncmp(f_name, CPIO_END, name_len))
            break;

        if (file_offset % 4 != 0) 
            file_offset = 4 * ((file_offset + 4) / 4);

        if (strncmp(f_name, name, name_len)){
            
            return f_size;
            // print_hex((uint32_t)file_content);

            // print_str("\nhere");
        }

        if(f_size % 4 != 0) 
            f_size = 4 * ((f_size + 4) / 4);

        head = (struct cpio_newc_header*)((uint8_t*)head + file_offset + f_size);
    }

    print_str("\nNo Match File");

    return 0;
}