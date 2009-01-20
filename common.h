/*****************************************************************************
* common.h: a frame-accurate screenshot generator.
*****************************************************************************
* Copyright (C) 2009
*
* Authors: Nathan Caldwell <saintdev@gmail.com>
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

#define MAX_FRAMES 1024

enum {
    COLORSPACE_420,
    COLORSPACE_422,
    COLORSPACE_444,
    COLORSPACE_444A
};

typedef void *handle_t;

typedef struct {
    int plane_cnt;
    int stride[4];
    uint8_t *plane[4];
} image_t;

typedef struct {
    /* pts of picture */
    int64_t pts;

    /* In: raw data */
    image_t img;
} picture_t;

typedef struct
{
    uint32_t width, height;
    int frame_cnt;
    int csp;
    uint32_t frames[MAX_FRAMES];
} config_t;
