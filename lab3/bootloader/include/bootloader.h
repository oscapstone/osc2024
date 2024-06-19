#ifndef	_BOOTLOADER_H
#define	_BOOTLOADER_H

int str_to_int(const char *val_str);
char *int_2_str(unsigned int val);
void load_kernel_img();

#endif  /*_BOOTLOADER_H */