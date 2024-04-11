int strcmp(char *str1, char *str2)
{
    char *ptr1 = str1;
    char *ptr2 = str2;
    while(1)
    {
        char c1 = *ptr1;
        ptr1++;
        char c2 = *ptr2;
        ptr2++;

        if(c1 == c2)
        {
            if(c1 == '\0') return 0;
            else continue;
        }
        else 
        {
            return 1;
        }
    }
}