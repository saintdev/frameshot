
#include <stdint.h>
#include <malloc.h>
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
