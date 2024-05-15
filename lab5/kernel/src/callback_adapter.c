#include <stdio.h>

typedef struct puts_args_struct
{
	char *str;
} puts_args_struct_t;

void adapter_puts(void *args_struct)
{
	puts_args_struct_t *puts_args_struct = (puts_args_struct_t *)args_struct;
	puts(puts_args_struct->str);
}

void adapter_timer_set2sAlert(void *args_struct){
	timer_set2sAlert();
}
