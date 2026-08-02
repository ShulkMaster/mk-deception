#ifndef MKD_PLATFORM_DISC_ERROR_H
#define MKD_PLATFORM_DISC_ERROR_H

int mwfile_error_callback(int operation, int error);
void check_handle_disc_error(void);

extern int disc_error_occurred;

#endif
