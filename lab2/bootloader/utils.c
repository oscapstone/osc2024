#include"header/utils.h"
#include"header/uart.h"

int string_compare(char* a, char* b) {
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

int atoi(char *str)
{
    int res = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        if (str[i] > '9' || str[i] < '0')
            return res;
        res = res * 10 + str[i] - '0';
    }

    return res;
}
