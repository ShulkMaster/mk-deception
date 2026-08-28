#ifndef CRI_MPS_H
#define CRI_MPS_H

typedef struct MpsPackHeader {
    long long scr;
    int is_mpeg1;
    int mux_rate;
} MpsPackHeader;

typedef struct MpsSystemHeader {
    int header_length;
    int rate_bound;
    int audio_bound;
    int video_bound;
    int fixed_flag;
    int csps_flag;
    int audio_lock_flag;
    int video_lock_flag;
} MpsSystemHeader;

typedef struct MpsPacketHeader {
    long long pts;
    long long dts;
    int stream_id;
    int stream_type;
    int stream_index;
    int packet_length;
    int std_buffer_size;
    int payload_length;
} MpsPacketHeader;

struct MpsHandle;

typedef void (*MpsErrorCallback)(int object);
typedef int (*MpsDecodeHeaderFn)(struct MpsHandle* handle, const unsigned char* data,
                                 int size, int* consumed, int* header_flags);

typedef union MpsHandlePayload {
    struct {
        MpsPackHeader pack_header;
        MpsSystemHeader last_system_header;
        MpsSystemHeader system_headers[3];
        MpsPacketHeader packet_header;
    } headers;
    int decoder_words[46];
} MpsHandlePayload;

typedef struct MpsHandle {
    int state;
    MpsErrorCallback error_callback;
    int error_object;
    int error;
    int format;
    int field_14;
    MpsHandlePayload payload;
    int field_D0;
    MpsDecodeHeaderFn decode_header;
    int field_D8;
    int field_DC;
    int field_E0;
    int field_E4;
    int field_E8;
    unsigned char field_EC[0x14];
} MpsHandle;

typedef struct MpsLibWork {
    MpsErrorCallback error_callback;
    int error_object;
    int error;
    int handle_count;
    MpsHandle handles[1];
} MpsLibWork;

int MPS_GetPketHd(MpsHandle* handle, MpsPacketHeader* out);
int MPS_GetLastSysHd(MpsHandle* handle, MpsSystemHeader* out);
int MPS_GetSysHd(MpsHandle* handle, MpsSystemHeader* out, int index);
int MPS_GetPackHd(MpsHandle* handle, MpsPackHeader* out);
void MPSGET_Finish(void);
void MPSGET_Init(void);

int MPSLIB_CheckHn(MpsHandle* handle);
int MPSLIB_SetErr(MpsHandle* handle, int error);
int MPS_Destroy(MpsHandle* handle);
MpsHandle* MPS_Create(void);
int MPS_SetErrFn(MpsHandle* handle, MpsErrorCallback callback, int object);
void MPS_Finish(void);
int MPS_Init(int handle_count, MpsLibWork* work);

extern MpsLibWork* MPSLIB_libwork;

#endif
