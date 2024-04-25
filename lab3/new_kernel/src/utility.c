#include "utility.h"
#include "mini_uart.h"

extern char __heap_top , __heap_bottom;
static char* htop_ptr = &__heap_top;

void* malloc(unsigned int size) {
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header

    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 16
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    
    return r;
}
void free(void* ptr) {
    // TBD
}

unsigned long long strlen(const char *str)
{
    int count = 0;
    while((unsigned char)*str++)count++;
    return count;
}
unsigned int vsprintf(char *dst, char* fmt, __builtin_va_list args)
{
    long int arg;
    int len, sign, i;
    char *p, *orig=dst, tmpstr[19];

    // failsafes
    if(dst==(void*)0 || fmt==(void*)0) {
        return 0;
    }

    // main loop
    arg = 0;
    while(*fmt) {
        if(dst-orig > 0x100-0x10)
        {
            return -1;
        }
        // argument access
        if(*fmt=='%') {
            fmt++;
            // literal %
            if(*fmt=='%') {
                goto put;
            }
            len=0;
            // size modifier
            while(*fmt>='0' && *fmt<='9') {
                len *= 10;
                len += *fmt-'0';
                fmt++;
            }
            // skip long modifier
            if(*fmt=='l') {
                fmt++;
            }
            // character
            if(*fmt=='c') {
                arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
                fmt++;
                continue;
            } else
            // decimal number
            if(*fmt=='d') {
                arg = __builtin_va_arg(args, int);
                // check input
                sign=0;
                if((int)arg<0) {
                    arg*=-1;
                    sign++;
                }
                if(arg>99999999999999999L) {
                    arg=99999999999999999L;
                }
                // convert to string
                i=18;
                tmpstr[i]=0;
                do {
                    tmpstr[--i]='0'+(arg%10);
                    arg/=10;
                } while(arg!=0 && i>0);
                if(sign) {
                    tmpstr[--i]='-';
                }
                if(len>0 && len<18) {
                    while(i>18-len) {
                        tmpstr[--i]=' ';
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            if(*fmt=='x') {
                arg = __builtin_va_arg(args, long int);
                i=16;
                tmpstr[i]=0;
                do {
                    char n=arg & 0xf;
                    tmpstr[--i]=n+(n>9?0x37:0x30);
                    arg>>=4;
                } while(arg!=0 && i>0);
                if(len>0 && len<=16) {
                    while(i>16-len) {
                        tmpstr[--i]='0';
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            if(*fmt=='s') {
                p = __builtin_va_arg(args, char*);
copystring:     if(p==(void*)0) {
                    p="(null)";
                }
                while(*p) {
                    *dst++ = *p++;
                }
            }
        } else {
put:        *dst++ = *fmt;
        }
        fmt++;
    }
    *dst=0;
    return dst-orig;
}

char* str_seperate(char* head)
{
    char* end;
    while(1){
        if(*head == '\0')
        {
            end = head;
            break;
        }
        if(*head == ' ')
        {
            *head = '\0';
            end = head + 1;
            break;
        }
        head++;
    }
    return end;
}

char* memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

char* strcpy (char *dest, const char *src)
{
    return memcpy (dest, src, strlen (src) + 1);
}

int atoi(char* str)
{
    // Initialize result
    int res = 0;

    // Iterate through all characters
    // of input string and update result
    // take ASCII character of corresponding digit and
    // subtract the code from '0' to get numerical
    // value and multiply res by 10 to shuffle
    // digits left to update running total
    for (int i = 0; str[i] != '\0'; ++i)
    {
        if(str[i] > '9' || str[i] < '0')return res;
        res = res * 10 + str[i] - '0';
    }

    // return result.
    return res;
}