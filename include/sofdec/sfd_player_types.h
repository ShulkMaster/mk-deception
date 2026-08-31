#ifndef MKD_SOFDEC_SFD_PLAYER_TYPES_H
#define MKD_SOFDEC_SFD_PLAYER_TYPES_H

typedef struct SfdHandle SfdHandle;
typedef void (*SfdTimerCallback)(void);
typedef int (*SfdTimeSourceFn)(SfdHandle* handle, int* value, int* scale);

typedef struct SfdPlaybackSettings {
    int values_00[5];
    int frame_rate_code;
    int values_18[10];
} SfdPlaybackSettings;

typedef struct SfdPlaybackRuntime {
    int decoded_pictures;
    int skipped_pictures;
    int field_08;
    int field_0C;
    int field_10;
    int field_14;
    int frame_outstanding;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    union {
        long long time_values[12];
        struct {
            long long reserved_time_values[3];
            struct {
                int high;
                unsigned int low;
            } input_flow;
            long long remaining_time_values[8];
        } timing;
    };
    int tail_values[4];
} SfdPlaybackRuntime;

typedef struct SfdTimerTimeUnit {
    int active;
    int fields_04[7];
    unsigned short field_20;
    unsigned short field_22;
    int value;
    int scale;
} SfdTimerTimeUnit;

typedef struct SfdTimerSkipState {
    SfdTimerCallback callback;
    int fields_04[7];
    unsigned short field_20;
    unsigned short field_22;
    int value;
    int scale;
} SfdTimerSkipState;

typedef struct SfdTimerCompactTimeUnit {
    int fields_00[6];
    unsigned short field_18;
    unsigned short field_1A;
    int value;
    int scale;
} SfdTimerCompactTimeUnit;

typedef struct SfdTimerStreamTimeUnit {
    int active;
    int field_04;
    int file_size;
    int total_time_value;
    int total_time_scale;
    int byte_rate;
    int seek_position;
    int field_1C;
    unsigned short field_20;
    unsigned short field_22;
    int value;
    int scale;
} SfdTimerStreamTimeUnit;

typedef struct SfdTimerSampleHistory {
    int fields_00[3];
    int samples[32];
} SfdTimerSampleHistory;

typedef struct SfdTimerSampleWindow {
    int enabled;
    int fields_04[3];
    int samples[32];
} SfdTimerSampleWindow;

typedef struct SfdFrameTime {
    unsigned char metadata[0x14];
    int value;
    int scale;
} SfdFrameTime;

typedef int (*SfdExternalClockFn)(int object, int* value, int* scale);

typedef struct SfdTimerState {
    SfdTimeSourceFn time_sources[6];
    SfdTimerSkipState skip_state;
    SfdTimerCompactTimeUnit compact_time;
    SfdTimerTimeUnit field_0068;
    SfdTimerStreamTimeUnit stream_time;
    SfdTimerTimeUnit field_00C0;
    SfdTimerTimeUnit video_start_time;
    SfdTimerTimeUnit elapsed_time;
    int start_time_value;
    int start_time_scale;
    int field_014C;
    long long field_0150;
    long long audio_start_pts;
    SfdTimerSampleHistory sample_history;
    SfdTimerSampleWindow sample_window;
    int video_end_time_value;
    int video_end_time_scale;
    int field_0284;
    int field_0288;
    int current_time_value;
    int current_time_scale;
    int interval_time_last;
    int interval_time_estimate;
    int interval_time_max;
    int interval_time_min;
    int field_02A4;
    int video_clock_sample;
    int speed;
    int field_02B0;
    int field_02B4;
    int field_02B8;
    int frame_time_repeat_count;
    float previous_frame_time;
    int previous_frame_ready;
    float last_frame_time;
    int field_02CC;
    int previous_clock_sample;
    SfdExternalClockFn external_clock_callback;
    int previous_external_sample;
    int current_clock_sample;
    int clock_sample_scale;
    int external_clock_wrap;
    int external_clock_object;
    unsigned char unknown_02EC[0x2C4];
    unsigned int video_pts[3];
    unsigned char unknown_05BC[0x24];
} SfdTimerState;

typedef struct SfdSeekState {
    SfdHandle* source_handle;
    int field_04;
    int field_08;
    int field_0C;
} SfdSeekState;

typedef char SfdPlaybackSettingsSizeCheck[
    sizeof(SfdPlaybackSettings) == 0x40 ? 1 : -1];
typedef char SfdPlaybackRuntimeSizeCheck[
    sizeof(SfdPlaybackRuntime) == 0xA0 ? 1 : -1];
typedef char SfdTimerTimeUnitSizeCheck[
    sizeof(SfdTimerTimeUnit) == 0x2C ? 1 : -1];
typedef char SfdTimerSkipStateSizeCheck[
    sizeof(SfdTimerSkipState) == 0x2C ? 1 : -1];
typedef char SfdTimerCompactTimeUnitSizeCheck[
    sizeof(SfdTimerCompactTimeUnit) == 0x24 ? 1 : -1];
typedef char SfdTimerStreamTimeUnitSizeCheck[
    sizeof(SfdTimerStreamTimeUnit) == 0x2C ? 1 : -1];
typedef char SfdTimerSampleHistorySizeCheck[
    sizeof(SfdTimerSampleHistory) == 0x8C ? 1 : -1];
typedef char SfdTimerSampleWindowSizeCheck[
    sizeof(SfdTimerSampleWindow) == 0x90 ? 1 : -1];
typedef char SfdFrameTimeSizeCheck[sizeof(SfdFrameTime) == 0x1C ? 1 : -1];
typedef char SfdTimerStateSizeCheck[
    sizeof(SfdTimerState) == 0x5E0 ? 1 : -1];
typedef char SfdSeekStateSizeCheck[
    sizeof(SfdSeekState) == 0x10 ? 1 : -1];

#endif
