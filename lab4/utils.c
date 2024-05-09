#include "include/utils.h"
#include "include/types.h"

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++, s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void str_int_to_decimal(char *buf, int value) {
  str_num_to_decimal(buf, value < 0 ? -value : value, value < 0);
}

void str_uint_to_decimal(char *buf, unsigned int value) {
  str_num_to_decimal(buf, value, 0);
}

void str_ulong_to_decimal(char *buf, unsigned long value) {
  str_num_to_decimal(buf, value, 0);
}

void str_num_to_decimal(char *buf, unsigned long value, int is_negative) {
  char *p = buf;
  if (value == 0) {
    *p++ = '0';
  }
  while (value > 0) {
    *p++ = (value % 10) + '0';
    value /= 10;
  }
  if (is_negative) {
    *p++ = '-';
  }
  *p = '\0';
  str_reverse(buf, p - 1);
}

void str_ulong_to_hex(char *buf, unsigned long value) {
  char *p = buf;
  int nibble;
  do {
    nibble = value & 0xF;
    *p++ = (nibble < 10) ? (nibble + '0') : (nibble - 10 + 'A');
    value >>= 4;
  } while (value != 0);
  *p = '\0';
  str_reverse(buf, p - 1);
}

void str_reverse(char *start, char *end) {
  char temp;
  while (start < end) {
    temp = *start;
    *start = *end;
    *end = temp;
    start++;
    end--;
  }
}

void vsnprintf(char *buf, unsigned int size, const char *fmt,
               __builtin_va_list args) {
  const char *p;
  char temp[20];
  int d;
  unsigned int u;
  char *s, *out = buf;
  char *buf_end = buf + size - 1;
  for (p = fmt; *p != '\0' && out < buf_end; ++p) {
    if (*p != '%') {
      *out++ = *p;
      continue;
    }
    switch (*++p) {
    case 'c':
      *out++ = (char)__builtin_va_arg(args, int);
      break;
    case 'd':
      d = __builtin_va_arg(args, int);
      str_int_to_decimal(temp, d);
      for (s = temp; *s && out < buf_end; ++s) {
        *out++ = *s;
      }
      break;
    case 'u':
      u = __builtin_va_arg(args, unsigned int);
      str_uint_to_decimal(temp, u);
      for (s = temp; *s && out < buf_end; ++s) {
        *out++ = *s;
      }
      break;
    case 'l':
      unsigned long ul = __builtin_va_arg(args, unsigned long);
      str_ulong_to_decimal(temp, ul);
      for (s = temp; *s && out < buf_end; ++s) {
        *out++ = *s;
      }
      break;
    case 'x':
      u = __builtin_va_arg(args, unsigned int);
      str_ulong_to_hex(temp, (unsigned long)u);
      for (s = temp; *s && out < buf_end; ++s) {
        *out++ = *s;
      }
      break;
    case 'p':
      unsigned long addr = __builtin_va_arg(args, unsigned long);
      str_ulong_to_hex(temp, addr);
      for (s = temp; *s && out < buf_end; ++s) {
        *out++ = *s;
      }
      break;
    case 's':
      s = __builtin_va_arg(args, char *);
      while (*s && out < buf_end) {
        *out++ = *s++;
      }
      break;
    default:
      *out++ = *p;
      break;
    }
  }
  *out = '\0';
}

int strncmp(const char *s1, const char *s2, unsigned int n) {
  while (n-- > 0) {
    if (*s1 != *s2) {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    if (*s1 == '\0') {
      break;
    }
    s1++;
    s2++;
  }
  return 0;
}

unsigned int strlen(const char *str) {
  const char *s;
  for (s = str; *s; ++s) {
  }
  return (unsigned int)(s - str);
}

char *strtok(char *str, const char *delimiters, char **saveptr) {
  if (str != NULL) {
    *saveptr = str;
  }
  if (*saveptr == NULL) {
    return NULL;
  }
  // Skip leading delimiters
  while (**saveptr && strchr(delimiters, **saveptr)) {
    (*saveptr)++;
  }
  if (**saveptr == '\0') {
    *saveptr = NULL;
    return NULL;
  }
  char *start_token = *saveptr;
  // Find the end of the token
  while (**saveptr && !strchr(delimiters, **saveptr)) {
    (*saveptr)++;
  }
  if (**saveptr == '\0') {
    *saveptr = NULL;
  } else {
    **saveptr = '\0';
    (*saveptr)++;
  }
  return start_token;
}

char *strchr(const char *str, int c) {
  while (*str != '\0') {
    if (*str == c) {
      return (char *)str;
    }
    str++;
  }
  return NULL;
}

int atoi(const char *str) {
  int res = 0;
  while (*str != '\0') {
    if (*str >= '0' && *str <= '9') {
      res = res * 10 + (*str - '0');
    } else {
      break;
    }
    str++;
  }
  return res;
}

void strcpy(char *dest, const char *src) {
  while (*src != '\0') {
    *dest++ = *src++;
  }
  *dest = '\0';
}

void strcat(char *dest, const char *src) {
  while (*dest != '\0') {
    dest++;
  }
  while (*src != '\0') {
    *dest = *src;
    dest++;
    src++;
  }
  *dest = '\0';
}

unsigned long str_to_hex(const char *str) {
  unsigned long result = 0;
  while (*str) {
    char c = *str++;
    int value = 0;
    if (c >= '0' && c <= '9') {
      value = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      value = 10 + (c - 'a');
    } else if (c >= 'A' && c <= 'F') {
      value = 10 + (c - 'A');
    } else {
      return 0;
    }
    result = result * 16 + value;
  }
  return result;
}