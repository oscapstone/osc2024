
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "uart1.h"

#define CYAN "\e[0;36m" // 青色
#define HRED "\e[0;91m"
#define CRESET "\e[0m"

#define WARING(fmt, ...) PRINT_MESSAGE(LEVEL_ERROR, "WARING", CYAN, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) PRINT_MESSAGE(LEVEL_ERROR, "ERROR", HRED, fmt, ##__VA_ARGS__)
#define PRINT_MESSAGE(level, prefix, color, fmt, ...)                    \
    do                                                                   \
    {                                                                    \
        uart_sendlinek(color "[" prefix "] " CRESET fmt, ##__VA_ARGS__); \
    } while (0)

#endif /* _DEBUG_H_ */