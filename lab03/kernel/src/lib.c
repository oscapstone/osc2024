#include "lib.h"
#include "mem.h"
#include "io.h"

uint32_t strtol(const char *sptr, uint32_t base, int size)
{
    uint32_t ret = 0;
    int i=0;
    
    while((sptr[i] != '\0') && (i<size)){
        if(base == 16){
            if(sptr[i] >= '0' && sptr[i] <= '9'){
                ret = ret * 16 + (sptr[i] - '0');
            }
            else if(sptr[i] >= 'a' && sptr[i] <= 'f'){
                ret = ret * 16 + (sptr[i] - 'a' + 10);
            }
            else if(sptr[i] >= 'A' && sptr[i] <= 'F'){
                ret = ret * 16 + (sptr[i] - 'A' + 10);
            }
            else{
                break;
            }
        }
        else if(base == 8 || base == 2){
            if(sptr[i] >= '0' && sptr[i] <= '9'){
                ret = ret * base + (sptr[i] - '0');
            }
            else{
                break;
            }
        }
        i++;
    }
    return ret;
}

int atoi(const char *str)
{
    int ret = 0;
    int i=0;
    while(str[i] != '\0' && (str[i] >= '0' && str[i] <= '9')){
        ret = ret * 10 + (str[i] - '0');
        i++;
    }
    return ret;
}

static char* HEAP_HEADER;
static char* HEAP_REAR;

void  mem_init()
{
    HEAP_HEADER = &_end;
    // HEAP_HEADER++;
    HEAP_REAR = HEAP_HEADER;
}

void* simple_malloc(uint32_t size)
{
    if((void*)HEAP_REAR + size > (void*)HEAP_END){
        // printf("\nMemory allocation failed: Out of memory");
        return (char*)0;
    }
    // printf("\nMemory allocation from: ");
    // printf_hex(HEAP_REAR);
    // printf(" to ");
    // printf_hex(((void*)HEAP_REAR + size - 1));
    // printf(", under limit ");
    // printf_hex(HEAP_END);
    char* ret = HEAP_REAR;
    HEAP_REAR += size;
    return ret;
}