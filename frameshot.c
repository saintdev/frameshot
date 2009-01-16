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
int (*open_infile) (char *filename, handle_t *handle);
int (*read_frame) (handle_t handle, picture_t *pic, int framenum);
int (*close_infile) (handle_t handle);

/* output file function pointers */
static int (*open_outfile) (char *filename, handle_t *handle, int compression);
static int (*write_image) (handle_t handle);
static int (*close_outfile) (handle_t handle);

static int parse_options(int argc, char **argv, cli_opt_t *opt);
static void show_help(void);

int main(int argc, char **argv)
{
    cli_opt_t opt;
    int fmt = 0;

    parse_options(argc, argv, &opt);

    return 0;
}

static int parse_options(int argc, char **argv, cli_opt_t *opt)
{
    int zlevel = Z_DEFAULT_COMPRESSION;

    memset(opt, 0, sizeof(*opt));

    /* Default output driver */
    open_outfile = open_file_png;
    close_outfile = close_file_png;

    for (;;) {
        int long_options_index = -1;
        static struct option long_options[] = {
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
}

static void show_help(void)
{
#define HELP printf
    HELP("Syntax: frameshot [options] infile <frames,...>\n"
         "\n"
         "Infile is a raw bitstream of one of the following codecs:\n"
         "  YUV4MPEG\n"
         "\n"
         "Options:\n"
         "\n"
         "  -h, --help                  Displays this message.\n"
        );
    HELP("  -o, --output <string>       Prefix to use for each output image.\n");
    HELP("  -z, --compression <integer> Ammount of compression to use.\n");
    HELP("  -1, --fast                  Use fastest compression.\n");
    HELP("  -9, --best                  Use best (slowest) compression.\n");
    HELP("\n");
}
