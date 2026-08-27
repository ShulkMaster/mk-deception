#include "dolphin/trk.h"

typedef unsigned long long u64;

enum {
    DS_NoError = 0,
    DS_NoMessageBufferAvailable = 0x300,
    DS_MessageBufferOverflow = 0x301,
    DS_MessageBufferReadError = 0x302,
};

extern BOOL gTRKBigEndian;
void* TRK_memcpy(void*, const void*, u32);
void* TRK_memset(void*, int, u32);

MessageBuffer gTRKMsgBufs[3];

static inline void TRKSetBufferUsed(MessageBuffer* message, BOOL used) {
    message->is_in_use = used;
}

static inline DSError TRKReadBuffer1_ui8(MessageBuffer* message, u8* value);
static inline DSError TRKReadBuffer1_ui32(MessageBuffer* message, u32* value);
static inline DSError TRKAppendBuffer1_ui8(MessageBuffer* message, u8 value);
static inline DSError TRKAppendBuffer1_ui32(MessageBuffer* message, u32 value);

DSError TRKInitializeMessageBuffers(void) {
    int index;

    for (index = 0; index < 3; index++) {
        TRKInitializeMutex(&gTRKMsgBufs[index]);
        TRKAcquireMutex(&gTRKMsgBufs[index]);
        TRKSetBufferUsed(&gTRKMsgBufs[index], 0);
        TRKReleaseMutex(&gTRKMsgBufs[index]);
    }
    return DS_NoError;
}

DSError TRKGetFreeBuffer(MessageBufferID* buffer_id, MessageBuffer** output) {
    DSError error = DS_NoMessageBufferAvailable;
    int index;

    *output = 0;
    for (index = 0; index < 3; index++) {
        MessageBuffer* message = TRKGetBuffer(index);
        TRKAcquireMutex(message);
        if (!message->is_in_use) {
            TRKResetBuffer(message, 1);
            TRKSetBufferUsed(message, 1);
            error = DS_NoError;
            *output = message;
            *buffer_id = index;
            index = 3;
        }
        TRKReleaseMutex(message);
    }
    if (error == DS_NoMessageBufferAvailable) {
        usr_puts_serial("ERROR : No buffer available\n");
    }
    return error;
}

void* TRKGetBuffer(MessageBufferID buffer_id) {
    MessageBuffer* message = 0;
    if (buffer_id >= 0 && buffer_id < 3) {
        message = &gTRKMsgBufs[buffer_id];
    }
    return message;
}

void TRKReleaseBuffer(MessageBufferID buffer_id) {
    if (buffer_id != -1 && buffer_id >= 0 && buffer_id < 3) {
        MessageBuffer* message = &gTRKMsgBufs[buffer_id];
        TRKAcquireMutex(message);
        TRKSetBufferUsed(message, 0);
        TRKReleaseMutex(message);
    }
}

void TRKResetBuffer(MessageBuffer* message, BOOL keep_data) {
    message->length = 0;
    message->position = 0;
    if (!keep_data) {
        TRK_memset(message->data, 0, sizeof(message->data));
    }
}

DSError TRKSetBufferPosition(MessageBuffer* message, u32 position) {
    DSError error = DS_NoError;
    if (position > sizeof(message->data)) {
        error = DS_MessageBufferOverflow;
    } else {
        message->position = position;
        if (position > message->length) {
            message->length = position;
        }
    }
    return error;
}

DSError TRKAppendBuffer(MessageBuffer* message, const void* data, u32 length) {
    DSError error = DS_NoError;
    u32 bytes_left;

    if (length == 0) {
        return DS_NoError;
    }
    bytes_left = sizeof(message->data) - message->position;
    if (bytes_left < length) {
        error = DS_MessageBufferOverflow;
        length = bytes_left;
    }
    if (length == 1) {
        message->data[message->position] = *(const u8*)data;
    } else {
        TRK_memcpy(message->data + message->position, data, length);
    }
    message->position += length;
    message->length = message->position;
    return error;
}

DSError TRKReadBuffer(MessageBuffer* message, void* data, u32 length) {
    DSError error = DS_NoError;
    u32 bytes_left;

    if (length == 0) {
        return DS_NoError;
    }
    bytes_left = message->length - message->position;
    if (length > bytes_left) {
        error = DS_MessageBufferReadError;
        length = bytes_left;
    }
    TRK_memcpy(data, message->data + message->position, length);
    message->position += length;
    return error;
}

static inline DSError TRKAppendBuffer1_ui8(MessageBuffer* message, u8 value) {
    if (message->position >= sizeof(message->data)) {
        return DS_MessageBufferOverflow;
    }
    message->data[message->position++] = value;
    message->length++;
    return DS_NoError;
}

static inline DSError TRKAppendBuffer1_ui32(MessageBuffer* message, u32 value) {
    u8* big_endian_data;
    u8* byte_data;
    u8 swap_buffer[sizeof(value)];

    if (gTRKBigEndian) {
        big_endian_data = (u8*)&value;
    } else {
        byte_data = (u8*)&value;
        big_endian_data = swap_buffer;
        big_endian_data[0] = byte_data[3];
        big_endian_data[1] = byte_data[2];
        big_endian_data[2] = byte_data[1];
        big_endian_data[3] = byte_data[0];
    }
    return TRKAppendBuffer(message, big_endian_data, sizeof(value));
}

DSError TRKAppendBuffer1_ui64(MessageBuffer* message, u64 value) {
    u8* big_endian_data;
    u8* byte_data;
    u8 swap_buffer[sizeof(value)];
    if (gTRKBigEndian) {
        big_endian_data = (u8*)&value;
    } else {
        byte_data = (u8*)&value;
        big_endian_data = swap_buffer;
        big_endian_data[0] = byte_data[7];
        big_endian_data[1] = byte_data[6];
        big_endian_data[2] = byte_data[5];
        big_endian_data[3] = byte_data[4];
        big_endian_data[4] = byte_data[3];
        big_endian_data[5] = byte_data[2];
        big_endian_data[6] = byte_data[1];
        big_endian_data[7] = byte_data[0];
    }
    return TRKAppendBuffer(message, big_endian_data, sizeof(value));
}

DSError TRKAppendBuffer_ui8(MessageBuffer* message, const u8* data, int count) {
    DSError error;
    int index;

    for (index = 0, error = DS_NoError;
         error == DS_NoError && index < count; index++) {
        error = TRKAppendBuffer1_ui8(message, data[index]);
    }
    return error;
}

DSError TRKAppendBuffer_ui32(MessageBuffer* message, const u32* data, int count) {
    DSError error;
    int index;

    for (index = 0, error = DS_NoError;
         error == DS_NoError && index < count; index++) {
        error = TRKAppendBuffer1_ui32(message, data[index]);
    }
    return error;
}

static inline DSError TRKReadBuffer1_ui8(MessageBuffer* message, u8* value) {
    return TRKReadBuffer(message, (void*)value, 1);
}

static inline DSError TRKReadBuffer1_ui32(MessageBuffer* message, u32* value) {
    DSError error;
    u8* big_endian_data;
    u8* byte_data;
    u8 swap_buffer[sizeof(*value)];

    if (gTRKBigEndian) {
        big_endian_data = (u8*)value;
    } else {
        big_endian_data = swap_buffer;
    }
    error = TRKReadBuffer(message, (void*)big_endian_data, sizeof(*value));
    if (!gTRKBigEndian && error == DS_NoError) {
        byte_data = (u8*)value;
        byte_data[0] = big_endian_data[3];
        byte_data[1] = big_endian_data[2];
        byte_data[2] = big_endian_data[1];
        byte_data[3] = big_endian_data[0];
    }
    return error;
}

DSError TRKReadBuffer1_ui64(MessageBuffer* message, u64* value) {
    DSError error;
    u8* big_endian_data;
    u8* byte_data;
    u8 swap_buffer[sizeof(*value)];

    if (gTRKBigEndian) {
        big_endian_data = (u8*)value;
    } else {
        big_endian_data = swap_buffer;
    }
    error = TRKReadBuffer(message, (void*)big_endian_data, sizeof(*value));
    if (!gTRKBigEndian && error == DS_NoError) {
        byte_data = (u8*)value;
        byte_data[0] = big_endian_data[7];
        byte_data[1] = big_endian_data[6];
        byte_data[2] = big_endian_data[5];
        byte_data[3] = big_endian_data[4];
        byte_data[4] = big_endian_data[3];
        byte_data[5] = big_endian_data[2];
        byte_data[6] = big_endian_data[1];
        byte_data[7] = big_endian_data[0];
    }
    return error;
}

DSError TRKReadBuffer_ui8(MessageBuffer* message, u8* data, int count) {
    DSError error;
    int index;

    for (index = 0, error = DS_NoError;
         error == DS_NoError && index < count; index++) {
        error = TRKReadBuffer1_ui8(message, &(data[index]));
    }
    return error;
}

DSError TRKReadBuffer_ui32(MessageBuffer* message, u32* data, int count) {
    DSError error;
    int index;

    for (index = 0, error = DS_NoError;
         error == DS_NoError && index < count; index++) {
        error = TRKReadBuffer1_ui32(message, &(data[index]));
    }
    return error;
}
