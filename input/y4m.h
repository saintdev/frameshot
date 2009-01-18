
int open_file_y4m(char *filename, handle_t *handle, config_t *config);
int read_frame_y4m(handle_t handle, picture_t *pic, int framenum);
int close_file_y4m(handle_t handle);
