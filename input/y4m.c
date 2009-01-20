#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <malloc.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "y4m.h"

/* Most of this is from x264 */

/* YUV4MPEG2 raw 420 yuv file operation */
typedef struct {
    FILE *fp;
    int width, height;
    int par_width, par_height;
    int next_frame;
    int seq_header_len, frame_header_len;
    int frame_size;
    int csp;
    int fps_num, fps_den;
} y4m_input_t;

#define Y4M_MAGIC "YUV4MPEG2"
#define MAX_YUV4_HEADER 80
#define Y4M_FRAME_MAGIC "FRAME"
#define MAX_FRAME_HEADER 80

int open_file_y4m(char *filename, handle_t *handle, config_t *config)
{
    int i, n, d;
    int interlaced;
    char header[MAX_YUV4_HEADER + 10];
    char *tokstart, *tokend, *header_end;
    y4m_input_t *h = calloc(1, sizeof(*h));

    h->next_frame = 0;

    if (!strcmp(filename, "-"))
        h->fp = stdin;
    else
        h->fp = fopen(filename, "rb");
    if (h->fp == NULL)
        return -1;

    h->frame_header_len = strlen(Y4M_FRAME_MAGIC) + 1;

    /* Read header */
    for (i = 0; i < MAX_YUV4_HEADER; i++) {
        header[i] = fgetc(h->fp);
        if (header[i] == '\n') {
            /* Add a space after last option. Makes parsing "444" vs
               "444alpha" easier. */
            header[i + 1] = 0x20;
            header[i + 2] = 0;
            break;
        }
    }
    if (i == MAX_YUV4_HEADER || strncmp(header, Y4M_MAGIC, strlen(Y4M_MAGIC)))
        return -1;

    /* Scan properties */
    header_end = &header[i + 1];        /* Include space */
    h->seq_header_len = i + 1;
    for (tokstart = &header[strlen(Y4M_MAGIC) + 1]; tokstart < header_end; tokstart++) {
        if (*tokstart == 0x20)
            continue;
        switch (*tokstart++) {
            case 'W':              /* Width. Required. */
                h->width = config->width = strtol(tokstart, &tokend, 10);
                tokstart = tokend;
                break;
            case 'H':              /* Height. Required. */
                h->height = config->height = strtol(tokstart, &tokend, 10);
                tokstart = tokend;
                break;
            case 'C':              /* Color space */
                if (strncmp("420", tokstart, 3)) {
                    fprintf(stderr, "Colorspace unhandled\n");
                    return -1;
                }
                tokstart = strchr(tokstart, 0x20);
                break;
            case 'I':              /* Interlace type */
                switch (*tokstart++) {
                    case 'p':
                        interlaced = 0;
                        break;
                    case '?':
                    case 't':
                    case 'b':
                    case 'm':
                    default:
                        interlaced = 1;
                        fprintf(stderr, "Warning, this sequence might be interlaced\n");
                }
                break;
            case 'F': /* Frame rate - 0:0 if unknown */
                      /* Frame rate in unimportant. */
                if (sscanf(tokstart, "%d:%d", &n, &d) == 2 && n && d) {
                    reduce_fraction(&n, &d);
                    h->fps_num = n;
                    h->fps_den = d;
                }
                tokstart = strchr(tokstart, 0x20);
                break;
            case 'A': /* Pixel aspect - 0:0 if unknown */
                      /* Don't override the aspect ratio if sar has been explicitly set on the commandline. */
                if (sscanf(tokstart, "%d:%d", &n, &d) == 2 && n && d) {
                    reduce_fraction(&n, &d);
                    h->par_width = n;
                    h->par_height = d;
                }
                tokstart = strchr(tokstart, 0x20);
                break;
            case 'X': /* Vendor extensions */
                if (!strncmp("YSCSS=", tokstart, 6)) {
                    /* Older nonstandard pixel format representation */
                    tokstart += 6;
                    if (strncmp("420JPEG", tokstart, 7) &&
                         strncmp("420MPEG2", tokstart, 8) &&
                         strncmp("420PALDV", tokstart, 8)) {
                        fprintf(stderr, "Unsupported extended colorspace\n");
                        return -1;
                    }
                }
                tokstart = strchr(tokstart, 0x20);
                break;
        }
    }

    fprintf(stderr, "yuv4mpeg: %ix%i@%i/%ifps, %i:%i\n",
            h->width, h->height, h->fps_num, h->fps_den,
            h->par_width, h->par_height);

    *handle = (handle_t)h;
    return 0;
}

int read_frame_y4m(handle_t handle, picture_t *pic, int framenum)
{
    int slen = strlen(Y4M_FRAME_MAGIC);
    int i = 0;
    char header[16];
    y4m_input_t *h = handle;

    if (framenum != h->next_frame) {
        if (fseek(h->fp, (uint64_t)framenum*(3*(h->width*h->height)/2+h->frame_header_len)
             + h->seq_header_len, SEEK_SET))
            return -1;
    }

    /* Read frame header - without terminating '\n' */
    if (fread(header, 1, slen, h->fp) != slen)
        return -1;

    header[slen] = 0;
    if (strncmp(header, Y4M_FRAME_MAGIC, slen)) {
        fprintf(stderr, "Bad header magic (%" PRIx32 " <=> %s)\n",
                *((uint32_t *) header), header);
        return -1;
    }

    /* Skip most of it */
    while (i < MAX_FRAME_HEADER && fgetc(h->fp) != '\n')
        i++;
    if (i == MAX_FRAME_HEADER) {
        fprintf(stderr, "Bad frame header!\n");
        return -1;
    }
    h->frame_header_len = i + slen + 1;

    if (fread(pic->img.plane[0], 1, h->width*h->height, h->fp) <= 0
        || fread(pic->img.plane[1], 1, h->width * h->height / 4, h->fp) <= 0
        || fread(pic->img.plane[2], 1, h->width * h->height / 4, h->fp) <= 0)
        return -1;

    pic->pts = framenum;
    h->next_frame = framenum + 1;

    return 0;
}

int close_file_y4m(handle_t handle)
{
    y4m_input_t *h = handle;
    if (!h || !h->fp)
        return 0;
    fclose(h->fp);
    free(h);
    return 0;
}
