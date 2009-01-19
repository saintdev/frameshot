#include "utils.h"

void reduce_fraction(int *n, int *d)
{
    int a = *n;
    int b = *d;
    int c;
    if (!a || !b)
        return;
    c = a % b;
    while (c) {
        a = b;
        b = c;
        c = a % b;
    }
    *n /= b;
    *d /= b;
}

int intcmp(const void *p1, const void *p2)
{
    int i1 = *(int *)p1;
    int i2 = *(int *)p2;
    if( i1 > i2 )
        return 1;
    else if( i1 < i2 )
        return -1;

    return 0;
}
