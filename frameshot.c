#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <zlib.h>

#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "common.h"
#include "output.h"
#include "input.h"

enum {
    FORMAT_UNKNOWN,
    FORMAT_H264,
    FORMAT_DIRAC,
    FORMAT_OGG,
    FORMAT_M4V
};

enum {
    COLORSPACE_420,
    COLORSPACE_422,
    COLORSPACE_444,
    COLORSPACE_444A
};

typedef struct {
    char *filein;
    handle_t hout;
    handle_t hin;
} cli_opt_t;

/* input file function pointers */
int (*open_infile) (char *filename, handle_t *handle, config_t *config);
int (*read_frame) (handle_t handle, picture_t *pic, int framenum);
int (*close_infile) (handle_t handle);

/* output file function pointers */
static int (*open_outfile) (char *filename, handle_t *handle, int compression);
// static int (*set_outfile_param) (handle_t handle, config_t *config);
static int (*write_image) (handle_t handle, picture_t *pic, config_t *config);
static int (*close_outfile) (handle_t handle);

static int parse_options(int argc, char **argv, config_t *config, cli_opt_t *opt);
static int grab_frames(config_t *config, cli_opt_t *opt);

int main(int argc, char **argv)
{
    config_t config;
    cli_opt_t opt;
    int ret = 0;

    parse_options(argc, argv, &config, &opt);

    ret = grab_frames(&config, &opt);

    return ret;
}

static void show_help(void)
{
#define HELP printf
    HELP("Syntax: frameshot [options] infile\n"
         "\n"
         "Infile is a raw bitstream of one of the following codecs:\n"
         "  YUV4MPEG\n"
         "\n"
         "Options:\n"
         "\n"
         "  -h, --help                  Displays this message.\n"
        );
    HELP("  -f, --frames <int,int,...>  Frames numbers to grab.\n");
    HELP("  -o, --output <string>       Prefix to use for each output image.\n");
    HELP("  -z, --compression <integer> Ammount of compression to use.\n");
    HELP("  -1, --fast                  Use fastest compression.\n");
    HELP("  -9, --best                  Use best (slowest) compression.\n");
    HELP("\n");
}

static int parse_options(int argc, char **argv, config_t *config, cli_opt_t *opt)
{
    char *filename = NULL;
    int zlevel = Z_DEFAULT_COMPRESSION;
    char *file_ext;
    int is_y4m = 0;

    memset(opt, 0, sizeof(*opt));

    /* Default input driver */
    open_infile = open_file_y4m;
    read_frame = read_frame_y4m;
    close_infile = close_file_y4m;

    /* Default output driver */
    open_outfile = open_file_png;
    write_image = write_image_png;
    close_outfile = close_file_png;

    for (;;) {
        int long_options_index = -1;
        static struct option long_options[] = {
            {"fast", no_argument, NULL, '1'},
            {"best", no_argument, NULL, '9'},
            {"frames", required_argument, NULL, 'f'},
            {"help", no_argument, NULL, 'h'},
            {"output", required_argument, NULL, 'o'},
            {"compression", required_argument, NULL, 'z'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "19f:ho:z:", long_options, &long_options_index);

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
            case 'f':

            case 'o':
                break;
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

    /* Get the input file name */
    if (optind > argc - 1) {
        fprintf(stderr, "[error]: No input file.\n");
        show_help();
        return -1;
    }
    filename = argv[optind++];

    file_ext = strrchr(filename, '.');
    if (!strncasecmp(file_ext, ".y4m", 4))
        is_y4m = 1;

    if (!opt->hout) {
        char *outname = strdup(filename);
        char *ext = strrchr(outname, '.');
        ext[1] = 'p';
        ext[2] = 'n';
        ext[3] = 'g';
        ext[4] = 0x00;
        open_outfile(outname, &opt->hout, zlevel);
        free(outname);
    }

    if (is_y4m) {
        open_infile = open_file_y4m;
        read_frame = read_frame_y4m;
        close_infile = close_file_y4m;
    }

    if (open_infile(filename, &opt->hin, config)) {
        fprintf(stderr, "ERROR: could not open input file '%s'\n", filename);
        return -1;
    }

    return 0;
}

static int grab_frames(config_t *config, cli_opt_t *opt)
{
    picture_t pic;

    pic.img.plane[0] = malloc(3 * config->width * config->height / 2);
    pic.img.plane[1] = pic.img.plane[0] + config->width * config->height;
    pic.img.plane[2] = pic.img.plane[1] + config->width * config->height / 4;
    pic.img.plane[3] = NULL;

    pic.img.stride[0] = config->width;
    pic.img.stride[1] = pic.img.stride[2] = config->width / 2;
    pic.img.stride[3] = 0;

    read_frame(opt->hin, &pic, 1);

    write_image(opt->hout, &pic, config);

    close_infile(opt->hin);
    close_outfile(opt->hout);

    return 0;
}
