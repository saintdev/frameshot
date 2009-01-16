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
