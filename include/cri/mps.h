#ifndef CRI_MPS_H
#define CRI_MPS_H

typedef int MpsCallbackObject;

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

typedef void (*MpsErrorCallback)(MpsCallbackObject object, int error);
typedef void (*MpsPesCallback)(MpsCallbackObject object,
                               unsigned char stream_id);

typedef struct MpsSystemStream {
    unsigned char stream_id;
    unsigned char buffer_bound_scale;
    unsigned short buffer_size_bound;
} MpsSystemStream;

typedef struct MpsSystemCallbackInfo {
    int rate_bound;
    unsigned char audio_bound;
    unsigned char fixed_flag;
    unsigned char csps_flag;
    unsigned char audio_lock_flag;
    unsigned char video_lock_flag;
    unsigned char video_bound;
    unsigned char packet_rate_restriction;
    unsigned char reserved_bits;
    int stream_count;
    MpsSystemStream streams[50];
} MpsSystemCallbackInfo;

typedef void (*MpsSystemCallback)(MpsCallbackObject object,
                                  MpsSystemCallbackInfo* info);
typedef void (*MpsPsMapCallback)(void);
typedef int (*MpsDecodeHeaderFn)(struct MpsHandle* handle, const unsigned char* data,
                                 int size, int* consumed, int* header_flags);

typedef struct MpsDecodedHeaders {
    MpsPackHeader pack_header;
    MpsSystemHeader last_system_header;
    MpsSystemHeader system_headers[3];
    MpsPacketHeader packet_header;
} MpsDecodedHeaders;

/**
 * Alternate views of the 0xB8-byte decoded-header storage. `MPS_Create`
 * initializes all 46 words together; decoder and getter paths use the typed
 * header view.
 */
typedef union MpsHandlePayload {
    MpsDecodedHeaders headers;
    int decoder_words[46];
} MpsHandlePayload;

typedef struct MpsHandle {
    int state;
    MpsErrorCallback error_callback;
    MpsCallbackObject error_object;
    int error;
    int packet_length_bytes;
    int field_14;
    MpsHandlePayload payload;
    int field_D0;
    MpsDecodeHeaderFn decode_header;
    int field_D8;
    int field_DC;
    int field_E0;
    MpsSystemCallback system_callback;
    MpsCallbackObject system_object;
    MpsPsMapCallback ps_map_callback;
    MpsCallbackObject ps_map_object;
    MpsPesCallback pes_callback;
    MpsCallbackObject pes_object;
    int field_FC;
} MpsHandle;

typedef struct MpsLibWork {
    MpsErrorCallback error_callback;
    MpsCallbackObject error_object;
    int error;
    int handle_count;
    MpsHandle handles[1];
} MpsLibWork;

typedef char MpsPackHeaderSizeCheck[sizeof(MpsPackHeader) == 0x10 ? 1 : -1];
typedef char MpsSystemHeaderSizeCheck[
    sizeof(MpsSystemHeader) == 0x20 ? 1 : -1];
typedef char MpsPacketHeaderSizeCheck[
    sizeof(MpsPacketHeader) == 0x28 ? 1 : -1];
typedef char MpsSystemStreamSizeCheck[
    sizeof(MpsSystemStream) == 0x04 ? 1 : -1];
typedef char MpsSystemCallbackInfoSizeCheck[
    sizeof(MpsSystemCallbackInfo) == 0xD8 ? 1 : -1];
typedef char MpsDecodedHeadersSizeCheck[
    sizeof(MpsDecodedHeaders) == 0xB8 ? 1 : -1];
typedef char MpsHandlePayloadSizeCheck[
    sizeof(MpsHandlePayload) == 0xB8 ? 1 : -1];
typedef char MpsHandleSizeCheck[sizeof(MpsHandle) == 0x100 ? 1 : -1];
typedef char MpsLibWorkSizeCheck[sizeof(MpsLibWork) == 0x110 ? 1 : -1];

int MPS_GetPketHd(MpsHandle* handle, MpsPacketHeader* out);
int MPS_GetLastSysHd(MpsHandle* handle, MpsSystemHeader* out);
int MPS_GetSysHd(MpsHandle* handle, MpsSystemHeader* out, int index);
int MPS_GetPackHd(MpsHandle* handle, MpsPackHeader* out);
void MPSGET_Finish(void);
void MPSGET_Init(void);

int MPS_CheckDelim(const unsigned char* data);
int MPSDEC_DecHdMpeg1(MpsHandle* handle, const unsigned char* data, int size,
                      int* consumed, int* header_flags);
void MPSDEC_Finish(void);
void MPSDEC_Init(void);

int MPSLIB_CheckHn(MpsHandle* handle);
int MPSLIB_SetErr(MpsHandle* handle, int error);
int MPS_Destroy(MpsHandle* handle);
MpsHandle* MPS_Create(void);
int MPS_SetErrFn(MpsHandle* handle, MpsErrorCallback callback,
                 MpsCallbackObject object);
int MPS_DecHd(MpsHandle* handle, const unsigned char* data, int size,
              int* consumed, int* header_flags);
int MPS_SetPesFn(MpsHandle* handle, MpsPesCallback callback,
                 MpsCallbackObject object);
int MPS_SetPsMapFn(MpsHandle* handle, MpsPsMapCallback callback,
                   MpsCallbackObject object);
int MPS_SetSystemFn(MpsHandle* handle, MpsSystemCallback callback,
                    MpsCallbackObject object);
void MPS_Finish(void);
int MPS_Init(int handle_count, MpsLibWork* work);

extern MpsLibWork* MPSLIB_libwork;

#endif
