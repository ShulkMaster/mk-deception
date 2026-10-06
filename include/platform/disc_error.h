#ifndef MKD_PLATFORM_DISC_ERROR_H
#define MKD_PLATFORM_DISC_ERROR_H

#include "mw/mwFile.h"

#ifdef __cplusplus
extern "C" {
#endif

void mwfile_error_callback(int operation, int error, const char* path,
                           mwFileCommand* command, void* context);
void check_handle_disc_error(void);

extern int disc_error_occurred;

#ifdef __cplusplus
}
#endif

#endif
