#ifndef MATH_H
#define MATH_H

int gcd(int, int);
int lcm(int, int);

#define LOG2LL(__value) \
    (((sizeof(long long) << 3) - 1) - __builtin_clzll((__value)))

#endif /* MATH_H */
