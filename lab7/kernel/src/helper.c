#include "mini_uart.h"

void hex_to_string(unsigned int x, char* buf) {
	buf[0] = '0'; buf[1] = 'x';
	char arr[16] = "0123456789abcdef";
	for (int i = 0; i < 8; i++) {
		unsigned char ch = (x >> ((32-4) - i*4)) & 0xF;
		buf[i + 2] = arr[ch];
	}
	buf[10] = '\0';
}

int same (char* one, char* two) {
	for(int i = 0; ; i ++) {
		if(one[i] == '\0' && two[i] == '\0') return 1;
		if(one[i] != two[i]) return 0;
	}
}

void substr(char* to, char* from, int l, int r) {
	for(int i = l; i <= r; i ++) {
		to[i - l] = from[i];
	}
	to[r - l + 1] = '\0';
}

int strlen(char* str) {
	int t = 0;
	while(str[t] != '\0') {
		t ++;
	}
	return t;
}

unsigned long stoi(char* str) {
	unsigned long res = 0;
	for(int i = 0; str[i] != '\0'; i ++) {
		res = res * 10 + str[i] - '0';
	}
	return res;
}

void strcpy(char* from, char* to, int size) {
	for (int i = 0; i < size; i ++) {
		to[i] = from[i];
	}
	to[size] = '\0';
}
void strcpy_to0(char* from, char* to) {
	for (int i = 0; ; i ++) {
		to[i] = from[i];
		if (to[i] == '\0') break;
	}
}

void delay (unsigned long long t) {
	while (t --);
}

void memset(char* arr, char t, int size) {
	for (int i = 0; i < size; i ++) {
		arr[i] = t;
	}
}
