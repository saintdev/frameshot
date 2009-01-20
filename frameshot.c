#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#include <zlib.h>


#define _GNU_SOURCE
#include <getopt.h>

#include "common.h"
#include "utils.h"
#include "output.h"
#include "input.h"

enum {
    FORMAT_UNKNOWN,
    FORMAT_H264,
    FORMAT_DIRAC,
    FORMAT_OGG,
    FORMAT_M4V
};

typedef struct {
    char *outdir;
    int zlevel;
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
    HELP("  -o, --outdir <string>       Output directory for images.\n");
    HELP("  -z, --compression <integer> Ammount of compression to use.\n");
    HELP("  -1, --fast                  Use fastest compression.\n");
    HELP("  -9, --best                  Use best (slowest) compression.\n");
    HELP("\n");
}

static int parse_options(int argc, char **argv, config_t *config, cli_opt_t *opt)
{
    char *filename = NULL;
    int zlevel = Z_DEFAULT_COMPRESSION;
    char *file_ext, *token;
    int is_y4m = 0;
    int is_dirac = 0;
    struct stat sb;

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
            {"outdir", required_argument, NULL, 'o'},
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
                for (config->frame_cnt = 0; config->frame_cnt < MAX_FRAMES; config->frame_cnt++, optarg = NULL) {
                    token = strtok(optarg, ",");
                    if (token == NULL)
                        break;
                    config->frames[config->frame_cnt] = atoi(token);
                }
                qsort(config->frames, config->frame_cnt, sizeof(*config->frames), intcmp);
                break;
            case 'o':
                opt->outdir = strdup(optarg);
                if (stat(opt->outdir, &sb) < 0) {
                    if (mkdir(opt->outdir, S_IRWXU) < 0) {
                        fprintf(stderr, "ERROR: Unable to create output directory.\n");
                        perror("mkdir");
                        return -1;
                    }
                } else {
                    if (!S_ISDIR(sb.st_mode)) {
                        fprintf(stderr, "ERROR: Outdir exists, and is not a directory.");
                        return -1;
                    }
                }
                break;
            case 'z':
                if (optarg == NULL || optarg[0] < '0' || optarg[0] > '9') {
                    opt->zlevel = Z_DEFAULT_COMPRESSION;
                    break;
                }
                opt->zlevel = atoi(optarg);
                break;
            case 'h':
            default:
                show_help();
                exit(0);
        }
    }

    /* Get the input file name */
    if (optind > argc - 1) {
        fprintf(stderr, "ERROR: No input file.\n");
        show_help();
        return -1;
    }
    filename = argv[optind++];

    file_ext = strrchr(filename, '.');
    if (!strncasecmp(file_ext, ".y4m", 4))
        is_y4m = 1;
    else if (!strncasecmp(file_ext, ".drc", 4))
        is_dirac = 1;

    if (!opt->outdir)
        opt->outdir = getcwd(NULL, 0);

    if (is_y4m) {
        open_infile = open_file_y4m;
        read_frame = read_frame_y4m;
        close_infile = close_file_y4m;
    }

    if (is_dirac) {
        open_infile = open_file_dirac;
        read_frame = read_frame_dirac;
        close_infile = close_file_dirac;
    }

    if (open_infile(filename, &opt->hin, config)) {
        fprintf(stderr, "ERROR: could not open input file '%s'\n", filename);
        return -1;
    }

    return 0;
}

static int grab_frames(config_t *config, cli_opt_t *opt)
{
    handle_t hout;
    picture_t pic;
    int i;
    char tmp[PATH_MAX];

    pic.img.plane[0] = calloc(1, 3 * config->width * config->height / 2);
    pic.img.plane[1] = pic.img.plane[0] + config->width * config->height;
    pic.img.plane[2] = pic.img.plane[1] + config->width * config->height / 4;
    pic.img.plane[3] = NULL;

    pic.img.stride[0] = config->width;
    pic.img.stride[1] = pic.img.stride[2] = config->width / 2;
    pic.img.stride[3] = 0;

    for (i = 0; i < config->frame_cnt; i++) {
        read_frame(opt->hin, &pic, config->frames[i]);

        snprintf(tmp, PATH_MAX, "%s/%05d.png", opt->outdir, config->frames[i]);
        open_outfile(tmp, &hout, opt->zlevel);
        write_image(hout, &pic, config);
        close_outfile(hout);
    }

    close_infile(opt->hin);

    if (opt->outdir)
        free(opt->outdir);

    return 0;
}
