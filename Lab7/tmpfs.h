struct tmpfs_node{
    char name[MAX_PATH];
    int type; // directory, mount, file
    struct vnode * entry[MAX_ENTRY];    
    char * data;
};

int reg_tmpfs();