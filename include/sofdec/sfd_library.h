#ifndef MKD_SOFDEC_SFD_LIBRARY_H
#define MKD_SOFDEC_SFD_LIBRARY_H

#include "sofdec/sfd_transport.h"

typedef struct SfdLibraryConfig {
    const SfdTransportRegistry* transport_registry;
    int timer_source;
} SfdLibraryConfig;

typedef struct SfdLibraryWork {
    int default_conditions[100];
    const SfdTransportRegistry* transport_registry_source;
    int timer_source;
    int field_0198;
    SfdErrorInfo error_info;
    SfdTimerLibraryWork timer_work;
    int buffer_work;
    SfdTransportRegistry transport_registry;
    int reset_in_progress;
    int server_handle_index;
    SfdHandle* handles[8];
} SfdLibraryWork;

typedef char SfdLibraryWorkSizeCheck[
    sizeof(SfdLibraryWork) == 0x224 ? 1 : -1];
typedef char SfdLibraryConfigSizeCheck[
    sizeof(SfdLibraryConfig) == 0x08 ? 1 : -1];

extern SfdLibraryWork SFLIB_libwork;
extern int sflib_sizeof_sfdhn;
extern void* SFD_pts_error_msg;
extern SfdHandle* sfd_hn_last;
extern const char SFLIB_version_str[0x54];

int SFD_Init(const SfdLibraryConfig* config);
int SFD_Finish(void);
int SFD_IsVersionCompatible(const char* version, int handle_size);

#endif
