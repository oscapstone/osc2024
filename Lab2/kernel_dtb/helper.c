int min(int n1, int n2)
{
    return n1 < n2 ? n1 : n2;
}

int string_length(char *s)
{
    int length = 0;

    for (; *s != '\0'; *s++, length++)
        ;

    return length;
}

char string_compare(char *s1, char *s2)
{
    int len1 = string_length(s1);
    int len2 = string_length(s2);

    if (len1 != len2)
    {
        return 0;
    }

    for (int i = 0; i < len1; i++)
    {
        if (s1[i] != s2[i])
        {
            return 0;
        }
    }

    return 1;
}

char string_start_with(char *full_str, char *target)
{
    int full_str_len = string_length(full_str);
    int target_len = string_length(target);

    if (target_len > full_str_len)
    {
        return 0;
    }

    for (int i = 0; i < target_len; i++)
    {
        if (full_str[i] != target[i])
        {
            return 0;
        }
    }

    return 1;
}

void string_split(char *str, char delimiter, char *result)
{
    int start_index = 0;

    while (str[start_index] != '\0' && str[start_index] != delimiter)
    {
        start_index++;
    }

    if (str[start_index] == '\0')
    {
        return;
    }

    int result_index = 0;
    for (int i = start_index + 1; i < string_length(str) && str[i] != delimiter; i++, result_index++)
    {
        result[result_index] = str[i];
    }

    result[result_index] = '\0';
}

void set(long addr, unsigned int value)
{
    volatile unsigned int *point = (unsigned int *)addr;
    *point = value;
}

void nop()
{
    asm volatile("nop");
}

unsigned int get_int(char *address)
{
    unsigned int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        sum += address[i] << (3 - i) * 8;
    }
    return sum;
}
