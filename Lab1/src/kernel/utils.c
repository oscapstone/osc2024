#include "kernel/utils.h"

char parse(const char c){
    if(c == CR)
        return '\n';
    else    
        return c;
}

int string_len(const char* str){
    int i = 0;

    while(1){
        if(*(str + i) == '\0' || *(str + i) == '\n')
            break;
        i++;
    }

    return i;
}
// According to strcmp in string.h, return 0 if strings are the same
int string_comp(const char *str1, const char *str2){
    // this should also be 1 or -1, but consider it as future work
    if(string_len(str1) != string_len(str2))
        return 1;
    else{
        int i;
        
        for(i = 0; i < string_len(str1); i++){
            // return >0 if str1 is larger in first unmatched char
            if(str1[i] > str2[i])
                return 1;
            // return <0 if str1 is smaller in first unmatched char
            else if(str1[i] < str2[i])
                return -1;
        }

        return 0;
    }
}

void string_set(char *str, int n, int size){
    int i;
    for(i = 0; i < size; i++)
        str[i] = n;
}