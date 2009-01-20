/*****************************************************************************
* utils.c: various utility functions.
*****************************************************************************
* Copyright (C) 2009
*
* Authors: Nathan Caldwell <saintdev@gmail.com>
*          x264 developers
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*****************************************************************************/

#include "utils.h"

/* Taken directly from x264 */
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
