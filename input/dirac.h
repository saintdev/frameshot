int open_file_dirac(char *filename, handle_t *handle, config_t *config);
int read_frame_dirac( handle_t handle, picture_t *pic, int framenum );
int close_file_dirac( handle_t handle );
