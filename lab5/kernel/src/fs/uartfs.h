#pragma once

#include "fs.h"

#define UART_FS_NAME        "uartfs"

FS_FILE_SYSTEM* uartfs_create();
