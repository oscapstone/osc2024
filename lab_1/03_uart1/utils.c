int strcmp(char* a, char* b){
    int value = 0;
    while(*a != '\0' && *b != '\0'){
        if(*a < *b){
            return -1;
        }
        else if(*a > *b){
            return 1;
        }
        else{
            a++;
            b++;
        }
    }
    if(*a == '\0'){
        value++;
    }
    if(*b == '\0'){
        value--;
    }
    return value;
}