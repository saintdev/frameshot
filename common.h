
typedef void *handle_t;

typedef struct {
    int csp;

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
