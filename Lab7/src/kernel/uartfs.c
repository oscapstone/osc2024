#include "kernel/uartfs.h"

struct file_operations dev_f_ops = {dev_uart_write, dev_uart_read, dev_uart_open, dev_uart_close, op_denied, op_denied};

int init_dev_uart(void){
    return register_devfs(&dev_f_ops);
}

int dev_uart_write(struct file *file, const void *buf, my_uint64_t len){
    for(int i = 0; i < len; i++){
        uart_irq_putc(((char*)buf)[i]);
    }
    return len;
}

int dev_uart_read(struct file *file, void *buf, my_uint64_t len){
    for(int i = 0; i < len; i++){
        ((char*)buf)[i] = uart_irq_getc();
    }
    return len;
}

int dev_uart_open(struct vnode *file_node, struct file **target){
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_f_ops;
    //(*target)->f_pos = 0;
    return 0;
}

int dev_uart_close(struct file *file){
    pool_free(file);
    return 0;
}