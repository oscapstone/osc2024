#include "vfs.h"
#include "framebufferfs.h"
#include "alloc.h"
#include "helper.h"
#include "mail.h"

extern char* cpio_base;

struct file_operations framebuffer_file_operations = {framebuffer_write,framebuffer_read,framebuffer_open,framebuffer_close};
struct vnode_operations framebuffer_vnode_operations = {framebuffer_lookup,framebuffer_create,framebuffer_mkdir};

vnode * framebuffer_create_vnode(const char* name, char* data, int size,
		unsigned int width, unsigned int height, unsigned int pitch,
		unsigned int isrgb, char* lfb) {
    vnode* node = my_malloc(sizeof(vnode));
    memset(node, 0, sizeof(vnode));
    node -> f_ops = &framebuffer_file_operations;
    node -> v_ops = &framebuffer_vnode_operations;
    
	framebuffer_node* inode = my_malloc(sizeof(framebuffer_node)); 
    memset(inode, 0, sizeof(framebuffer_node));
    strcpy(name, inode -> name, strlen(name));
	inode -> width = width;
	inode -> height = height;
	inode -> pitch = pitch;
	inode -> isrgb = isrgb;
	inode -> lfb = lfb;
    node -> internal = inode;
	// not necessary tho
    
	return node;
}

#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8
#define MBOX_TAG_LAST 0


int framebuffer_mount(struct filesystem *_fs, struct mount *mt){

	unsigned int __attribute__((aligned(16))) mbox[36];
	mbox[0] = 35 * 4;
	mbox[1] = MBOX_REQUEST;

	mbox[2] = 0x48003; // set phy wh
	mbox[3] = 8;
	mbox[4] = 8;
	mbox[5] = 1024; // FrameBufferInfo.width
	mbox[6] = 768;  // FrameBufferInfo.height

	mbox[7] = 0x48004; // set virt wh
	mbox[8] = 8;
	mbox[9] = 8;
	mbox[10] = 1024; // FrameBufferInfo.virtual_width
	mbox[11] = 768;  // FrameBufferInfo.virtual_height

	mbox[12] = 0x48009; // set virt offset
	mbox[13] = 8;
	mbox[14] = 8;
	mbox[15] = 0; // FrameBufferInfo.x_offset
	mbox[16] = 0; // FrameBufferInfo.y.offset

	mbox[17] = 0x48005; // set depth
	mbox[18] = 4;
	mbox[19] = 4;
	mbox[20] = 32; // FrameBufferInfo.depth

	mbox[21] = 0x48006; // set pixel order
	mbox[22] = 4;
	mbox[23] = 4;
	mbox[24] = 1; // RGB, not BGR preferably

	mbox[25] = 0x40001; // get framebuffer, gets alignment on request
	mbox[26] = 8;
	mbox[27] = 8;
	mbox[28] = 4096; // FrameBufferInfo.pointer
	mbox[29] = 0;    // FrameBufferInfo.size

	mbox[30] = 0x40008; // get pitch
	mbox[31] = 4;
	mbox[32] = 4;
	mbox[33] = 0; // FrameBufferInfo.pitch

	mbox[34] = MBOX_TAG_LAST;

	uart_printf ("wait for mailbox setup\r\n");

	// this might not return exactly what we asked for, could be
	// the closest supported resolution instead
	if (mailbox_call(mbox, MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
	  mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
		mt -> fs = _fs;
		const char * fname = "framebuffer";
		mt -> root = framebuffer_create_vnode(fname, 0, 0, 
			mbox[5], mbox[6], mbox[33], mbox[24], mbox[28] + 0xffff000000000000);
		uart_printf ("lfb: %llx\r\n", mbox[28] + 0xffff000000000000);
	  /*
	  m_width = mbox[5];        // get actual physical width
	  m_height = mbox[6];       // get actual physical height
	  m_pitch = mbox[33];       // get number of bytes per line
	  m_isrgb = mbox[24];       // get the actual channel order
	  lfb = (void *)((unsigned long)mbox[28]);
	  */
	} 
	else {
	  uart_printf ("Unable to set screen resolution to 1024x768x32\n");
	}
	
	uart_printf ("finish mailbox setup\r\n");	

    return 0;
}

int framebuffer_write(struct file *file, const char *buf, size_t len){
	// uart_printf ("lol: ");
	char* lfb = ((framebuffer_node*)(file -> vnode -> internal)) -> lfb;
	char* f = lfb + file -> f_pos;
	// uart_printf ("[FRAMEBUFFER]Writing to %llx\r\n", f);
    for (int i = 0; i < len; i ++) {
		f[i] = buf[i];
	}
	file -> f_pos += len;
	return len;
}

int framebuffer_read(struct file *file, char *buf, size_t len){
	char* lfb = ((framebuffer_node*)(file -> vnode -> internal)) -> lfb;
	char* f = lfb + file -> f_pos;
	// uart_printf ("[FRAMEBUFFER]Reading from %llx\r\n", f);
    for (int i = 0; i < len; i++) {
    	buf[i] = f[i];
	}
	file -> f_pos += len;
    return len;
}
// 27b81c
int framebuffer_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
	(*target) -> ref = 1;
    return 0;
}

int framebuffer_close(struct file *file){
    // my_free(file);
	uart_printf ("Shouldn't happen\r\n");
	return -1;
}

int framebuffer_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int framebuffer_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int framebuffer_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}
