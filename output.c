/*****************************************************************************
* output.c: output drivers.
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

#include <stdint.h>
#include <malloc.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <png.h>

#include "common.h"
#include "output.h"

typedef struct {
    FILE *fp;
    png_structp png;
    png_infop info;
} png_output_t;

int open_file_png(char *filename, handle_t *handle, int compression)
{
    png_output_t *h = NULL;
    if ((h = calloc(1, sizeof(*h))) == NULL)
        return -1;

    if (!strcmp(filename, "-"))
        h->fp = stdout;
    else if ((h->fp = fopen(filename, "wb")) == NULL) {
        goto error;
    }

    if ((h->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
        goto error;
    }

    if ((h->info = png_create_info_struct(h->png)) == NULL) {
        png_destroy_write_struct(&(h->png), (png_infopp) NULL);
        goto error;
    }

    png_init_io(h->png, h->fp);

    png_set_compression_level(h->png, compression);

    *handle = h;

    return 0;

error:
    if (h->fp != NULL && h->fp != stdout)
        fclose(h->fp);
    free(h);

    return -1;
}

int close_file_png(handle_t handle)
{
    int ret = 0;
    png_output_t *h = handle;

    png_destroy_write_struct(&(h->png), &(h->info));

    if ((h->fp == NULL) || (h->fp == stdout))
        return ret;

    ret = fclose(h->fp);

    free(h);

    return ret;
}

int write_image_png(handle_t handle, picture_t *pic, config_t *config)
{
    png_output_t *h = handle;
    uint8_t *out_data = malloc(config->width * config->height * 4);
    uint8_t **rows = calloc(config->height, sizeof(*rows));
    picture_t pic_out;
    struct SwsContext *sws_ctx;
    int i;

    pic_out.img.plane[0] = out_data;
    pic_out.img.plane[1] = pic_out.img.plane[2] = pic_out.img.plane[3] = NULL;
    pic_out.img.stride[0] = 3 * config->width;
    pic_out.img.stride[1] = pic_out.img.stride[2] = pic_out.img.stride[3] = 0;

    sws_ctx = sws_getContext(config->width, config->height, PIX_FMT_YUV420P,
                             config->width, config->height, PIX_FMT_RGB24,
                             SWS_FAST_BILINEAR | SWS_ACCURATE_RND,
                             NULL, NULL, NULL);

    sws_scale(sws_ctx, pic->img.plane, pic->img.stride, 0, config->height, pic_out.img.plane, pic_out.img.stride);

    __asm__ volatile ("emms\n\t");

    png_set_IHDR(h->png, h->info, config->width, config->height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    for (i = 0; i < config->height; i++)
        rows[i] = pic_out.img.plane[0] + i * pic_out.img.stride[0];

    png_set_rows(h->png, h->info, rows);

    png_write_png(h->png, h->info, 0, NULL);

    free(rows);
    free(out_data);

    return 0;
}
