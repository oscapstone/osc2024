#include "peripherals/mbox.h"

#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm.h"

/* mailbox message buffer */
volatile uint32_t __attribute__((aligned(16))) mbox[36];

static filesystem *fbfs = (filesystem *)0;
static vnode_operations *fbfs_v_ops = (vnode_operations *)0;
static file_operations *fbfs_f_ops = (file_operations *)0;
static uint32_t width, height, pitch, isrgb; /* dimensions and channel order */
static uint8_t *lfb;                         /* raw frame buffer address */

uint32_t mbox_call(uint8_t ch) {
  uint32_t r = ((uint32_t)((uint64_t)&mbox) | (ch & 0xF));
  /* wait until we can write to the mailbox */
  do {
    asm volatile("nop");
  } while (*MBOX_STATUS & MBOX_FULL);
  /* write the address of our message to the mailbox with channel identifier */

  *MBOX_WRITE = r;
  /* now wait for the response */

  while (1) {
    /* is there a response? */
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_EMPTY);
    /* is it a response to our message? */
    if (r == *MBOX_READ) /* is it a valid successful response? */
      return (mbox[1] == MBOX_CODE_BUF_RES_SUCC);
  }
  return 0;
}

void get_arm_base_memory_sz() {
  mbox[0] = 8 * 4;              // length of the message
  mbox[1] = MBOX_CODE_BUF_REQ;  // this is a request message

  mbox[2] = MBOX_TAG_GET_ARM_MEM;  // get serial number command
  mbox[3] = 8;                     // buffer size
  mbox[4] = MBOX_CODE_TAG_REQ;
  mbox[5] = 0;  // clear output buffer
  mbox[6] = 0;

  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP)) {
    uart_send_string("ARM memory base address: ");
    uart_hex(mbox[5]);
    uart_send_string("\r\n");

    uart_send_string("ARM memory size: ");
    uart_hex(mbox[6]);
    uart_send_string("\r\n");

  } else {
    uart_puts("Unable to query arm memory and size..");
  }
}

void get_board_serial() {
  // get the board's unique serial number with a mailbox call
  mbox[0] = 8 * 4;              // length of the message
  mbox[1] = MBOX_CODE_BUF_REQ;  // this is a request message

  mbox[2] = MBOX_TAG_GET_BOARD_SERIAL;  // get serial number command
  mbox[3] = 8;                          // buffer size
  mbox[4] = MBOX_CODE_TAG_REQ;
  mbox[5] = 0;  // clear output buffer
  mbox[6] = 0;

  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP)) {
    uart_send_string("Borad serial number: ");
    uint64_t num = ((uint64_t)mbox[6] << 32) | ((uint64_t)mbox[5]);
    uart_hex_64(num);
    uart_send_string("\r\n");
  } else {
    uart_puts("Unable to query serial number..");
  }
}

void get_board_revision() {
  mbox[0] = 7 * 4;  // buffer size in bytes
  mbox[1] = MBOX_CODE_BUF_REQ;

  mbox[2] = MBOX_TAG_GET_BOARD_REVISION;  // tag identifier
  mbox[3] = 4;  // maximum of request and response value buffer's length.
  mbox[4] = MBOX_CODE_TAG_REQ;
  mbox[5] = 0;  // value buffer

  mbox[6] = MBOX_TAG_LAST;  // tags end

  if (mbox_call(MBOX_CH_PROP)) {
    uart_send_string("Board revision: ");  // it should be 0xa020d3 for rpi3 b+
    uart_hex(mbox[5]);
    uart_send_string("\r\n");
  } else {
    uart_puts("Unable to query board revision..");
  }
}

static file *fbfs_create_fd(vnode *file_node) {
  file *fp = (file *)malloc(sizeof(file));
  if (!fp) {
    return (file *)0;
  }
  fp->vnode = file_node;
  fp->f_ops = file_node->f_ops;
  fp->f_pos = 0;
  return fp;
}

static vnode *fbfs_create_vnode(vnode *parent, const char *name, uint8_t type) {
  vnode *fbfs_vnode = (vnode *)malloc(sizeof(vnode));
  if (!fbfs_vnode) {
    return (vnode *)0;
  }
  strcpy(fbfs_vnode->name, name);
  fbfs_vnode->parent = parent;
  if (parent) {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (parent->subdirs[i] == (vnode *)0) {
        parent->subdirs[i] = fbfs_vnode;
        break;
      }
    }
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    fbfs_vnode->subdirs[i] = (vnode *)0;
  }
  fbfs_vnode->mountpoint = (mount *)0;
  fbfs_vnode->v_ops = fbfs_v_ops;
  fbfs_vnode->f_ops = fbfs_f_ops;
  fbfs_vnode->type = type;
  fbfs_vnode->internal = (void *)0;

  return fbfs_vnode;
}

static int32_t fbfs_vnode_create(vnode *dir_node, vnode **target,
                                 const char *component_name) {
  uart_puts("create operation fails on fbfs");
  return 0;
}

static int32_t fbfs_vnode_lookup(vnode *dir_node, vnode **target,
                                 const char *component_name) {
  uart_puts("lookup operation fails on fbfs");
  return 0;
}

static int32_t fbfs_vnode_mkdir(vnode *dir_node, vnode **target,
                                const char *component_name) {
  uart_puts("mkdir operation fails on fbfs");
  return 0;
}

static int32_t fbfs_file_open(vnode *file_node, file **fp) {
  *fp = fbfs_create_fd(file_node);
  if (!(*fp)) {
    return -1;
  }
  return 0;
}

static int32_t fbfs_file_close(file *fp) {
  free((void *)fp);
  return 0;
}

static int32_t fbfs_file_read(file *fp, void *buf, uint32_t len) {
  uart_puts("read operation fails on fbfs");
  return 0;
}

static int32_t fbfs_file_write(file *fp, const void *buf, uint32_t len) {
  const char *src = (const char *)buf;
  uint32_t i;
  for (i = 0; i < len; i++) {
    lfb[fp->f_pos++] = src[i];
  }
  return i;
}

static int32_t fbfs_setup_mount(filesystem *fs, mount *mount,
                                const char *name) {
  mount->fs = fs;
  mount->root = fbfs_create_vnode((vnode *)0, name, DEVICE);
  if (!mount->root) {
    return -1;
  }
  mount->root->mountpoint = mount;

  mbox[0] = 35 * 4;
  mbox[1] = MBOX_CODE_BUF_REQ;

  mbox[2] = 0x48003;  // set phy wh
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024;  // FrameBufferInfo.width
  mbox[6] = 768;   // FrameBufferInfo.height

  mbox[7] = 0x48004;  // set virt wh
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024;  // FrameBufferInfo.virtual_width
  mbox[11] = 768;   // FrameBufferInfo.virtual_height

  mbox[12] = 0x48009;  // set virt offset
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0;  // FrameBufferInfo.x_offset
  mbox[16] = 0;  // FrameBufferInfo.y.offset

  mbox[17] = 0x48005;  // set depth
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32;  // FrameBufferInfo.depth

  mbox[21] = 0x48006;  // set pixel order
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1;  // RGB, not BGR preferably

  mbox[25] = 0x40001;  // get framebuffer, gets alignment on request
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096;  // FrameBufferInfo.pointer
  mbox[29] = 0;     // FrameBufferInfo.size

  mbox[30] = 0x40008;  // get pitch
  mbox[31] = 4;
  mbox[32] = 4;
  mbox[33] = 0;  // FrameBufferInfo.pitch

  mbox[34] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
    mbox[28] &= 0x3FFFFFFF;  // convert GPU address to ARM address
    width = mbox[5];         // get actual physical width
    height = mbox[6];        // get actual physical height
    pitch = mbox[33];        // get number of bytes per line
    isrgb = mbox[24];        // get the actual channel order
    lfb = (uint8_t *)phy2vir((uint64_t)mbox[28]);
  } else {
    uart_puts("Unable to set screen resolution to 1024x768x32\n");
    return -1;
  }

  return 0;
}

static int32_t fbfs_register_fs() {
  if (!fbfs_v_ops) {
    fbfs_v_ops = (vnode_operations *)malloc(sizeof(vnode_operations));
    if (!fbfs_v_ops) {
      return -1;
    }
    fbfs_v_ops->create = (vfs_create_fp_t)phy2vir((uint64_t)fbfs_vnode_create);
    fbfs_v_ops->lookup = (vfs_lookup_fp_t)phy2vir((uint64_t)fbfs_vnode_lookup);
    fbfs_v_ops->mkdir = (vfs_mkdir_fp_t)phy2vir((uint64_t)fbfs_vnode_mkdir);
  }

  if (!fbfs_f_ops) {
    fbfs_f_ops = (file_operations *)malloc(sizeof(file_operations));
    if (!fbfs_f_ops) {
      return -1;
    }
    fbfs_f_ops->open = (vfs_open_fp_t)phy2vir((uint64_t)fbfs_file_open);
    fbfs_f_ops->close = (vfs_close_fp_t)phy2vir((uint64_t)fbfs_file_close);
    fbfs_f_ops->read = (vfs_read_fp_t)phy2vir((uint64_t)fbfs_file_read);
    fbfs_f_ops->write = (vfs_write_fp_t)phy2vir((uint64_t)fbfs_file_write);
  }
  return 0;
}

filesystem *fbfs_init() {
  if (!fbfs) {  // if the fbfs has not been initialized
    fbfs = (filesystem *)malloc(sizeof(filesystem));
    strcpy(fbfs->name, "fbfs");
    fbfs->setup_mnt = (vfs_setup_mount)phy2vir((uint64_t)fbfs_setup_mount);
    fbfs->register_fs = (vfs_register_fs)phy2vir((uint64_t)fbfs_register_fs);
  }
  return fbfs;
}