
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
