#ifndef _FIRMWARE_H
#define _FIRMWARE_H

#include "int.h"

void board_cmd(char *args, size_t arg_size);
void reboot_cmd(char *args, size_t arg_size);

#endif  // _FIRMWRARE_H
