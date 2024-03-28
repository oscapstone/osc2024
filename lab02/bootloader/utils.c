#include "utils.h"
#include "uart.h"

#define BUFSIZE 1024


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


void print_char(const char c) {
  if (c == '\n') mini_uart_send('\r');
  mini_uart_send(c);
}

void print_string(const char *str) {
  while (*str) {
    print_char(*str++);
  }
}


void print_h(unsigned int num) {
  print_string("0x");
  int h = 28;
  while (h >= 0) {
    char ch = (num >> h) & 0xF;
    if (ch >= 10) ch += 'A' - 10;
    else ch += '0';
    print_char(ch);
    h -= 4;
  }
}



void print_num(int num) {
  if (num == 0) {
    print_char('0');
    return;
  }
  if (num < 0) {
    print_char('-');
    num = -num;
  }
  char buf[10];
  int len = 0;
  while (num > 0) {
    buf[len++] = (char)(num%10)+'0';
    num /= 10;
  }
  for (int i = len-1; i >= 0; i--) {
    print_char(buf[i]);
  }
}

char read_c(){
    return mini_uart_recv();
}

unsigned int strlen(const char *s) {
    unsigned int len = 0;

    while (*s != '\0') {
        len++; s++;
    }

    return len;
}



int strncmp(const char *s1, const char *s2, uint32_t len) {
  for (int i = 0; i < len; i++) {
    if (s1[i] > s2[i]) return 1;
    else if (s1[i] < s2[i]) return -1;
    else if (s1[i] == '\0' || s2[i] == '\0') break;
  }
  return 0;
}

void* memset(char *dst, char c, uint32_t len) {
  char* dup = dst;
  while (len--) {
    *dup++ = c;
  }
  return dst;
}


char* strncpy(char *dst, const char *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = src[i];
    if (src[i] == 0) break;
  }
  return dst;
}


int strcmp(char *a, char *b){

    char *p1=a;
    char *p2=b;
    while(1){
        char c1 = *p1;
        p1++;
        char c2 = *p2;
        p2++;

        if(c1==c2){
            if(c1 == '\0') return 1;
            else continue;
        }
        else return 0;

    }



}