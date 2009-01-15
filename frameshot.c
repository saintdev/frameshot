#include <stdlib.h>

#include <string.h>
#include <png.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

#define _GNU_SOURCE
#include <getopt.h>

enum {
    FORMAT_UNKNOWN,
    FORMAT_H264,
    FORMAT_DIRAC,
    FORMAT_OGG,
    FORMAT_M4V
};

typedef void *handle_t;

typedef struct {
    char *filein;
    handle_t hout;
    handle_t hin;
} cli_opt_t;

/* input file function pointers */
int (*open_infile) (char *filename, handle_t * handle);
int (*read_frame) (handle_t handle);
int (*close_infile) (handle_t handle);

/* output file function pointers */
static int (*open_outfile) (char *filename, handle_t * handle);
static int (*write_image) (handle_t handle);
static int (*close_outfile) (handle_t handle);

int main(int argc, char **argv)
{
    int fmt = 0;

    parse_options(argc, argv);

    fmt = guess_format(input_file);

    check_format(input_file, fmt);

    switch (fmt) {
        case FORMAT_H264:
            break;
        case FORMAT_DIRAC:
            break;
        case FORMAT_OGG:
            break;
        case FORMAT_M4V:
            break;
        default:
    }
}

static int parse_options(int argc, char **argv, cli_opt_t * opt)
{
    int zlevel = Z_DEFAULT_COMPRESSION;

    memset(opt, 0, sizeof(*opt));

    /* Default output driver */
    open_outfile = open_file_png;
    close_outfile = close_file_png;

    for (;;) {
        int long_options_index = -1;
        static struct option logn_options[] = {
            {"help", no_argument, NULL, 'h'},
            {"output", required_argument, NULL, 'o'},
            {"compression", required_argument, NULL, 'z'},
            {"best", no_argument, NULL, '9'},
            {"fast", no_argument, NULL, '1'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "19ho:z:", long_options, &long_options_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case '1':
                zlevel = Z_BEST_SPEED;
                break;
            case '9':
                zlevel = Z_BEST_COMPRESSION;
                break;
            case 'o':
                if (!strcmp(optarg, "-"))

            case 'z':
                    zlevel = atoi(optarg);
                if (zlevel < 0 || zlevel > 9)
                    zlevel = Z_DEFAULT_COMPRESSION;
                break;
            case 'h':
            default:
                show_help();
                exit(0);
        }
    }
}

typedef struct {
    FILE *fp;
    png_structp png;
    png_infop info;
} png_output_t;

int open_file_png(char *filename, handle_t * handle, int zlevel)
{
    png_output_t *h = NULL;
    if ((h = calloc(1, sizeof(*h))) == NULL)
        return -1;

    if (!strcmp(filename, "-"))
        h->fp = stdout;
    else if ((h->fp = fopen(filename, "wb")) == NULL) {
        goto cleanup;
    }

    if ((h->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
        goto cleanup;
    }

    if ((h->info = png_create_info_struct(h->png)) == NULL) {
        png_destroy_write_struct(&(h->png), (png_infopp) NULL);
        goto cleanup;
    }

    png_init_io(h->png, h->fh);

    png_set_compression_level(h->png, zlevel);

    *handle = h;

    return 0;

  cleanup:
    if (h->fh != NULL && h->fh != stdout)
        fclose(h->fh);
    free(h);

    return -1;
}

int close_file_png(handle_t handle)
{
    int ret = 0;
    png_output_t *h = handle;

    png_write_struct_destroy(&(h->png), &(h->info));

    if ((h->fh == NULL) || (h->fh == stdout))
        return ret;

    ret = fclose(h->fh);

    free(h);

    return ret;
}
