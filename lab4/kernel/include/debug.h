#include <uart1.h>

#define LEVEL_ERROR 0
#define LEVEL_WARNING 1
#define LEVEL_INFO 2
#define LEVEL_DEBUG 3

#include <uart1.h>

#define LEVEL_ERROR 0
#define LEVEL_WARNING 1
#define LEVEL_INFO 2
#define LEVEL_DEBUG 3

#ifndef _DEBUG
#define _DEBUG 0
#endif

#define PRINT_MESSAGE(level, prefix, fmt, ...)         \
	do                                                 \
	{                                                  \
		uart_puts("[" prefix "] " fmt, ##__VA_ARGS__); \
	} while (0)

#define NOTHING_TO_DO() \
	do                  \
	{                   \
	} while (0)

#define DO_CONTENT(content) \
	do                      \
	{                       \
		content             \
	} while (0)

#if _DEBUG >= LEVEL_ERROR
#define ERROR(fmt, ...) PRINT_MESSAGE(LEVEL_ERROR, "ERROR", fmt, ##__VA_ARGS__)
#define ERROR_BLOCK(content) DO_CONTENT(content)
#else
#define ERROR(fmt, ...) NOTHING_TO_DO()
#define ERROR_BLOCK(content) NOTHING_TO_DO()
#endif

#if _DEBUG >= LEVEL_WARNING
#define WARNING(fmt, ...) PRINT_MESSAGE(LEVEL_WARNING, "WARNING", fmt, ##__VA_ARGS__)
#define WARNING_BLOCK(content) DO_CONTENT(content)
#else
#define WARNING(fmt, ...) NOTHING_TO_DO()
#define WARNING_BLOCK(content) NOTHING_TO_DO()
#endif

#if _DEBUG >= LEVEL_INFO
#define INFO(fmt, ...) PRINT_MESSAGE(LEVEL_INFO, "INFO", fmt, ##__VA_ARGS__)
#define INFO_BLOCK(content) DO_CONTENT(content)
#else
#define INFO(fmt, ...) NOTHING_TO_DO()
#define INFO_BLOCK(content) NOTHING_TO_DO()
#endif

#if _DEBUG >= LEVEL_DEBUG
#define DEBUG(fmt, ...) PRINT_MESSAGE(LEVEL_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define DEBUG_BLOCK(content) DO_CONTENT(content)
#else
#define DEBUG(fmt, ...) NOTHING_TO_DO()
#define DEBUG_BLOCK(content) NOTHING_TO_DO()
#endif
