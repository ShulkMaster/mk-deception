#ifndef MKD_SOFDEC_SFD_TRANSPORT_H
#define MKD_SOFDEC_SFD_TRANSPORT_H

#include "cri/sj.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_header.h"
#include "sofdec/sfd_player_types.h"
#include "sofdec/sfd_timer.h"

typedef struct SfdTransportInterface SfdTransportInterface;
typedef struct SfdTransportSetup SfdTransportSetup;
typedef struct SfdVideoFrameState SfdVideoFrameState;

typedef struct SfdBufferChannel {
    SJ* stream_joint;
    int object;
    int state_08;
    int state_0C;
} SfdBufferChannel;

typedef struct SfdPtsQueue {
    void* entries;
    int capacity;
    int count;
    int write_index;
    int read_index;
} SfdPtsQueue;

typedef struct SfdBufferRingWork {
    int field_00;
    SJ* stream_joint;
    unsigned char* buffer;
    int buffer_size;
    int field_10;
    int field_14;
    int delimiter_position;
    int delimiter_size;
    int write_total;
    int read_total;
    SfdPtsQueue pts_queue;
} SfdBufferRingWork;

typedef struct SfdBufferTransfer {
    SJCK chunks[2];
    int field_10;
    int field_14;
    int field_18;
} SfdBufferTransfer;

typedef struct SfdBufferSupply {
    int field_00;
    SJ* stream_joint;
    unsigned char* buffer;
    int buffer_size;
    int field_10;
    int field_14;
} SfdBufferSupply;

typedef struct SfdBufferLinearWork {
    unsigned char* buffer;
    int buffer_size;
    int field_08;
    int field_0C;
    SfdVideoFrameState* video_frames;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
} SfdBufferLinearWork;

typedef union SfdBufferWork {
    SfdBufferRingWork ring;
    SfdBufferLinearWork linear;
    SfdBufferChannel user_channels[3];
    unsigned char raw[0x3C];
} SfdBufferWork;

typedef int (*SfdTransportLifecycleFn)(SfdHandle* handle);
typedef int (*SfdTransportTransferFn)(SfdHandle* handle, int parameter,
                                      int value);
typedef int (*SfdTransportBufferFn)(SfdHandle* handle, void* buffer);

struct SfdTransportInterface {
    SfdTransportLifecycleFn init;
    SfdTransportLifecycleFn finish;
    SfdTransportLifecycleFn exec_server;
    SfdTransportLifecycleFn create;
    SfdTransportLifecycleFn destroy;
    SfdTransportLifecycleFn standby;
    SfdTransportLifecycleFn start;
    SfdTransportLifecycleFn stop;
    SfdTransportLifecycleFn pause;
    SfdTransportBufferFn get_write;
    SfdTransportTransferFn add_write;
    SfdTransportBufferFn get_read;
    SfdTransportTransferFn add_read;
    SfdTransportTransferFn seek;
};

typedef struct SfdBufferState {
    int storage_mode;
    int active;
    int prepared;
    int terminated;
    SfdBufferWork work;
    int input_transport;
    int output_transport;
    unsigned char unknown_0054[0x20];
} SfdBufferState;

struct SfdVideoFrameState {
    int state;
    unsigned char unknown_0004[0x84];
};

typedef struct SfdBufferCreateConfig {
    const SfdTransportSetup* transport_setup;
    unsigned char* memory;
    int buffer_sizes[7];
    int field_24;
    int ring_alignment;
} SfdBufferCreateConfig;

typedef struct SfdCreateConfig {
    SfdBufferCreateConfig buffer;
    unsigned char unknown_002C[0x10];
    unsigned char* handle_memory;
    int handle_memory_size;
} SfdCreateConfig;

typedef struct SfdTransportState {
    int prepared;
    int terminated;
    void* context;
    const SfdTransportInterface* interface;
    int parameter_10;
    int parameter_14;
    int parameter_18;
    int parameter_1C;
    int state;
    unsigned char unknown_0024[0x20];
} SfdTransportState;

typedef struct SfdUserOutputWork {
    int state;
    SfdBufferChannel channels[3];
} SfdUserOutputWork;

struct SfdTransportSetup {
    const SfdTransportInterface* entries[9];
};

typedef struct SfdTransportRegistry {
    const SfdTransportInterface* entries[15];
} SfdTransportRegistry;

struct SfdHandle {
    SfdCreateConfig create_config;
    int field_0044;
    int playback_state;
    int requested_state;
    int field_0050;
    int field_0054;
    unsigned char unknown_0058[0x20];
    SfdHeaderState header_state;
    unsigned char unknown_010C[0x800];
    SfdPlaybackSettings playback_settings;
    int field_094C;
    SfdPlaybackRuntime playback_runtime;
    SfdErrorInfo error_info;
    int conditions_primary[100];
    int conditions_secondary[100];
    int field_0D24;
    SfdTimerState timer_state;
    SfdBufferState buffers[8];
    SfdVideoFrameState video_frames[16];
    SfdTransportState transports[9];
    unsigned char unknown_218C[0x1304];
    SfdUserOutputWork user_output_work;
    int field_34C4;
    SfdSeekState seek_state;
    SfdTimerSummary timer_summaries[6];
};

typedef char SfdTransportInterfaceSizeCheck[
    sizeof(SfdTransportInterface) == 0x38 ? 1 : -1];
typedef char SfdBufferChannelSizeCheck[
    sizeof(SfdBufferChannel) == 0x10 ? 1 : -1];
typedef char SfdPtsQueueSizeCheck[sizeof(SfdPtsQueue) == 0x14 ? 1 : -1];
typedef char SfdBufferRingWorkSizeCheck[
    sizeof(SfdBufferRingWork) == 0x3C ? 1 : -1];
typedef char SfdBufferTransferSizeCheck[
    sizeof(SfdBufferTransfer) == 0x1C ? 1 : -1];
typedef char SfdBufferSupplySizeCheck[
    sizeof(SfdBufferSupply) == 0x18 ? 1 : -1];
typedef char SfdBufferLinearWorkSizeCheck[
    sizeof(SfdBufferLinearWork) == 0x3C ? 1 : -1];
typedef char SfdBufferWorkSizeCheck[
    sizeof(SfdBufferWork) == 0x3C ? 1 : -1];
typedef char SfdBufferStateSizeCheck[
    sizeof(SfdBufferState) == 0x74 ? 1 : -1];
typedef char SfdVideoFrameStateSizeCheck[
    sizeof(SfdVideoFrameState) == 0x88 ? 1 : -1];
typedef char SfdTransportStateSizeCheck[
    sizeof(SfdTransportState) == 0x44 ? 1 : -1];
typedef char SfdUserOutputWorkSizeCheck[
    sizeof(SfdUserOutputWork) == 0x34 ? 1 : -1];
typedef char SfdTransportSetupSizeCheck[
    sizeof(SfdTransportSetup) == 0x24 ? 1 : -1];
typedef char SfdBufferCreateConfigSizeCheck[
    sizeof(SfdBufferCreateConfig) == 0x2C ? 1 : -1];
typedef char SfdCreateConfigSizeCheck[
    sizeof(SfdCreateConfig) == 0x44 ? 1 : -1];
typedef char SfdTransportRegistrySizeCheck[
    sizeof(SfdTransportRegistry) == 0x3C ? 1 : -1];
typedef char SfdHandleSizeCheck[
    sizeof(SfdHandle) == 0x3598 ? 1 : -1];

void SFPTS_InitPtsQue(SfdPtsQueue* queue);
void SFSET_SetCond(SfdHandle* handle, int condition, int value);
int SFBUF_GetTermFlg(SfdHandle* handle, int buffer_index);
int SFBUF_RingGetDataSiz(SfdHandle* handle, int buffer_index);
int SFBUF_GetWTot(SfdHandle* handle, int buffer_index);
int SFBUF_GetRTot(SfdHandle* handle, int buffer_index);
int SFBUF_GetPrepFlg(SfdHandle* handle, int buffer_index);
void SFBUF_SetPrepFlg(SfdHandle* handle, int buffer_index, int prepared);
void SFBUF_SetUoch(SfdHandle* handle, int buffer_index, int channel,
                   const SfdBufferChannel* config);
int SFBUF_RingAddWrite(SfdHandle* handle, int buffer_index, int parameter,
                       int value);
int SFBUF_RingGetWrite(SfdHandle* handle, int buffer_index, void* buffer);
int SFBUF_InitHn(SfdHandle* handle, SfdBufferState* buffers,
                 const SfdBufferCreateConfig* create);
void SFBUF_DestroySj(SfdHandle* handle);
void SFBUF_Init(int* work);
void SFBUF_Finish(int* work);
int SFTRN_GetTermFlg(SfdHandle* handle, int transport_index);
void SFTRN_SetTermFlg(SfdHandle* handle, int transport_index, int terminated);
int SFTRN_GetPrepFlg(SfdHandle* handle, int transport_index);
void SFTRN_SetPrepFlg(SfdHandle* handle, int transport_index, int prepared);
int SFTRN_IsSetup(SfdHandle* handle, int transport_index);
int SFTRN_CallTrtTrif(SfdHandle* handle, int transport_index,
                      int callback_index, int parameter, int value);
int SFTRN_CallTrSetup(SfdHandle* handle, int callback_index);
int SFTRN_InitHn(SfdHandle* handle, SfdTransportState* transports,
                 const SfdBufferCreateConfig* create,
                 const void* buffer_setup);
int SFTRN_Finish(SfdTransportRegistry* registry);
int SFTRN_Init(SfdTransportRegistry* registry,
               const SfdTransportRegistry* source);

int SFD_SetUsrSj(SfdHandle* handle, int channel, SJ* stream_joint,
                 int object);

extern const SfdTransportInterface SFD_tr_in_mem;
extern const SfdTransportInterface SFD_tr_uo;

#endif
