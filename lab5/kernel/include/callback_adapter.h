#include <stdio.h>

typedef struct puts_args_struct
{
	char *str;
} puts_args_struct_t;

void adapter_puts(void *args_struct);
void adapter_timer_set2sAlert(void *args_struct);
