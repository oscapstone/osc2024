// #include <stdio.h>

int strcmp(char *a, char *b) //return 0 if two string(char array) is equal
{
    while (*a)
    {
        if (*a != *b)
        {
            break;
        }
        a++;
        b++;
    }
    return *a - *b; //return first element difference of the char array
}

// int main(){
//     char *a = "fuck";
//     char *b = "";

//     int c = strcmp(a, b);

//     printf("%d\n", c);

// }