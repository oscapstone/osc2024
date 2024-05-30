#include "vfs.h"
#include "memory.h"

extern char current_dir[MAX_PATH];

int tmpfs_mount(struct filesystem *fs, struct mount *mt){
    mt -> fs = fs;
    mt -> root = 
}

int reg_tmpfs(){
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_mount;
    return register_filesystem(&fs);
}