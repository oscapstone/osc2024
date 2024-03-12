
int strcmp( char *s1, char *s2)
{
    int val = 0;

    while ( *s1 != '\0' && *s2 != '\0')
    {
        if ( val != 0)
        {
            break;
        }// if
        val = ( *s1 != *s2);
        s1 += 1;
        s2 += 1;
    }// while

    return val;
}

int strlen( char *s)
{
    int len = 0;
    while ( *s != '\0')
    {
        s += 1;
        len += 1;
    }// while

    return len;
}
