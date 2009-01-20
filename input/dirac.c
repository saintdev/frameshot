#include <stdio.h>
#include <string.h>
#include <schroedinger/schro.h>
#include "common.h"
#include "dirac.h"

typedef struct {
    FILE *fp;
    SchroDecoder *schro;
    SchroVideoFormat *format;
} dirac_input_t;

#define DIRAC_PARSE_MAGIC "BBCD"

int parse_packet(dirac_input_t *h, uint8_t **data, int *pkt_size);
static void buffer_free(SchroBuffer *buf, void *priv);

int open_file_dirac(char *filename, handle_t *handle, config_t *config)
{
    int size = -1;
    int it;
    uint8_t *packet = NULL;
    dirac_input_t *h = calloc(1, sizeof(*h));
    SchroBuffer *buffer;

    if (!strcmp(filename, "-"))
        h->fp = stdin;
    else
        h->fp = fopen(filename, "rb");
    if (h->fp == NULL)
        return -1;

    if (parse_packet(h, &packet, &size))
        return -1;

    if (size == 0)
        return -1;

    schro_init();

    h->schro = schro_decoder_new();

    buffer = schro_buffer_new_with_data(packet, size);
    buffer->free = buffer_free;
    buffer->priv = packet;

    it = schro_decoder_push(h->schro, buffer);
    if (it == SCHRO_DECODER_FIRST_ACCESS_UNIT) {
        h->format = schro_decoder_get_video_format(h->schro);
    }

    if (h->format->interlaced)
        fprintf(stderr, "warning: sequence may be interlaced!\n");

    config->width = h->format->width;
    config->height = h->format->height;
    switch (h->format->chroma_format) {
        case SCHRO_CHROMA_420:
            config->csp = COLORSPACE_420;
            break;
        case SCHRO_CHROMA_444:
        case SCHRO_CHROMA_422:
            fprintf(stderr, "ERROR: Unsupported chroma format.\n");
            return -1;
    }

    *handle = (handle_t)h;
    return 0;
}

int read_frame_dirac(handle_t handle, picture_t *pic, int framenum)
{
    dirac_input_t *h = handle;
    int size = -1;
    uint8_t *packet;
    SchroBuffer *buffer;
    SchroFrame *frame;
    int go = 1;

    /* This function assumes that it will be called with framenum increasing. */
    schro_decoder_set_earliest_frame(h->schro, framenum);

    while (1) {
        /* Handle EOF from previous iteration */
        if (size == 0)
            break;

        go = 1;
        while (go) {
            switch (schro_decoder_wait(h->schro)) {
                case SCHRO_DECODER_NEED_BITS:
                    go = 0;
                    break;
                case SCHRO_DECODER_NEED_FRAME:
                    switch (h->format->chroma_format) {
                        case SCHRO_CHROMA_444:
                            frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_444,
                                                              h->format->width, h->format->height);
                            break;
                        case SCHRO_CHROMA_422:
                            frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_422,
                                                              h->format->width, h->format->height);
                            break;
                        case SCHRO_CHROMA_420:
                            frame = schro_frame_new_and_alloc(NULL, SCHRO_FRAME_FORMAT_U8_420,
                                                              h->format->width, h->format->height);
                            break;
                        default:
                            printf("ERROR: unsupported chroma format\n");
                            return -1;
                    }
                    schro_decoder_add_output_picture(h->schro, frame);
                    break;
                case SCHRO_DECODER_OK:
                    {
                        int dts = schro_decoder_get_picture_number(h->schro);
                        frame = schro_decoder_pull(h->schro);
                        if (dts != framenum) {
                            /* This shouldn't happen, why does it? */
                            schro_frame_unref(frame);
                            break;
                        }

                        memcpy(pic->img.plane[0], frame->components[0].data, frame->components[0].length);
                        memcpy(pic->img.plane[1], frame->components[1].data, frame->components[1].length);
                        memcpy(pic->img.plane[2], frame->components[2].data, frame->components[2].length);

                        schro_frame_unref(frame);
                        return 0;
                    }
                    break;
                case SCHRO_DECODER_EOS:
                    schro_decoder_reset(h->schro);
                    go = 0;
                    break;
                case SCHRO_DECODER_ERROR:
                    break;
            }
        }

        if (parse_packet(h, &packet, &size)) {
            break;
        }

        if (size == 0) {
            /* Unexpected EOF */
            schro_decoder_push_end_of_stream(h->schro);
            return -1;
        } else {
            buffer = schro_buffer_new_with_data(packet, size);
            buffer->free = buffer_free;
            buffer->priv = packet;

            schro_decoder_push(h->schro, buffer);
        }
    }

    return 0;
}

int close_file_dirac(handle_t handle)
{
    dirac_input_t *h = handle;
    if (!h || !h->fp || !h->schro || !h->format)
        return 0;
    fclose(h->fp);
    free(h->format);
    schro_decoder_free(h->schro);
    free(h);
    return 0;
}

/* From schroedinger-tools */
int parse_packet(dirac_input_t * h, uint8_t **data, int *pkt_size)
{
    uint8_t *packet;
    uint8_t header[13];
    int n;
    int size;

    n = fread(header, 1, 13, h->fp);
    if (feof(h->fp)) {
        *data = NULL;
        *pkt_size = 0;
        return 0;
    }
    if (n < 13) {
        fprintf(stderr, "ERROR: truncated header\n");
        return -1;
    }

    if (strncmp((char *)header, DIRAC_PARSE_MAGIC, strlen(DIRAC_PARSE_MAGIC))) {
        fprintf(stderr, "ERROR: header magic incorrect\n");
        return -1;
    }

    size = (header[5] << 24) | (header[6] << 16) | (header[7] << 8) | (header[8]);
    if (size == 0) {
        size = 13;
    }
    if (size < 13) {
        fprintf(stderr, "ERROR: packet too small? (%d)\n", size);
        return -1;
    }
    if (size > 1 << 24) {
        fprintf(stderr, "ERROR: packet too large? (%d > 1<<24)\n", size);
        return -1;
    }

    packet = malloc(size);
    memcpy(packet, header, 13);
    n = fread(packet + 13, 1, size - 13, h->fp);
    if (n < size - 13) {
        free(packet);
        return -1;
    }

    *data = packet;
    *pkt_size = size;
    return 0;
}

static void buffer_free(SchroBuffer * buf, void *priv)
{
    free(priv);
}
