
int open_file_png(char *filename, handle_t *handle, int compression);
int write_image_png(handle_t handle, picture_t *pic, config_t *config);
int close_file_png(handle_t handle);
