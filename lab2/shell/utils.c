#include "header/utils.h"
#include "header/uart.h"

void read_cmd(char* command) {
    char input_char;
    int idx = 0;
    command[0] = '\0';
    while((input_char = uart_get_char()) != '\n') {
        command[idx] = input_char;
        idx += 1;
        uart_send_char(input_char);
    }
    command[idx] = '\0';
}

int strcmp(const char* command, const char* b) {
    while(*command != '\0' && *b != '\0') {
        if(*command != *b) {
            return 0;
        }
        command++;
        b++;
    }
    return (*command == '\0' && *b == '\0');
}

int strncmp(const char* command, const char* b, int n) {
    while(n > 0 && *command != '\0' && *b != '\0') {
        if(*command != *b) {
            return 0;
        }
        command++;
        b++;
        n--;
    }
    return (n == 0 || (*command == '\0' && *b == '\0'));
}

int atoi(const char *s, unsigned int size) {
    int num = 0;

    for (unsigned int i = 0; i < size && s[i] != '\0'; i++) {
        if ('0' <= s[i] && s[i] <= '9') {
            num += s[i] - '0';
        } else if ('A' <= s[i] && s[i] <= 'F') {
            num += s[i] - 'A' + 10;
        } else if ('a' <= s[i] && s[i] <= 'f') {
            num += s[i] - 'a' + 10;
        }
    }

    return num;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    } else {
        return c;
    }
}

int isxdigit(int c) {
    return isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F');
}

unsigned int parse_hex_str(char *arr, int size) {
    unsigned int result = 0;

    for (int i = 0; i < size; i++) {
        char c = arr[i];
        if (isxdigit(c)) {
            int val = isdigit(c) ? c - '0' : toupper(c) - 'A' + 10;
            result = (result << 4) + val;
        }
    }

    return result;
}

int read_buf(char *buf, int len){
    char c;
    int i;
    for (i = 0; i < len; i++) {
        c = uart_get_char();
        if (c == 127) { i--; continue; }
        uart_send_char(c);
        // print_num((int)c);
        if (c == '\r') {
        c = '\n';
        uart_send_char('\n');
        break;
        }
        buf[i] = c;
    }
    buf[i] = '\0';
    return i;
}

int memcmp(const void *cs, const void *ct, unsigned long count){
 
    const unsigned char *su1, *su2;
    int res = 0;
 
    for (su1=cs, su2=ct; 0<count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
 
    return res;
}

int hex2int(char *s, int n)
{
    int r = 0;
    // uart_send_string("\r\n");
    while (n-- > 0)
    {
        // uart_send_char(*s);
        r <<= 4;
        if (*s >= '0' && *s <= '9')
            r += *s++ - '0';
        else if (*s >= 'A' && *s <= 'F')
            r += *s++ - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f')
            r += *s++ - 'a' + 10;
    }
    return r;
}

char* strncpy(char *dst, const char *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = src[i];
    if (src[i] == 0) break;
  }
  return dst;
}

// For devtree

uint32_t utils_align_up(uint32_t size, int alignment) {
  return (size + alignment - 1) & ~(alignment-1);
}

int utils_string_compare(const char* str1,const char* str2) {
  for(;*str1 !='\0'||*str2 !='\0';str1++,str2++){
    if(*str1 != *str2) return 0;
    else if(*str1 == '\0' && *str2 =='\0') return 1;
  }
  return 1;
}

uint32_t utils_strlen(const char *s) {
    uint32_t i = 0;
	while (s[i]) i++;
	return i+1;
}