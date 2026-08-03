#ifndef MKD_PLATFORM_DISC_ERROR_H
#define MKD_PLATFORM_DISC_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

int mwfile_error_callback(int operation, int error);
void check_handle_disc_error(void);

extern int disc_error_occurred;

#ifdef __cplusplus
}
#endif

#endif
