typedef unsigned char u8;
typedef unsigned long u32;
typedef int DSError;
typedef int MessageBufferID;
typedef struct MessageBuffer MessageBuffer;

typedef struct TRKEvent {
    int event_type;
    u32 event_id;
    int message_buffer_id;
} TRKEvent;

typedef struct TRKFramingState {
    int message_buffer_id;
    u32 receive_count;
    int receive_state;
    int is_escape;
    u32 checksum;
} TRKFramingState;

typedef union PacketHeader {
    u8 bytes[0x40];
    u32 size;
} PacketHeader;

static TRKFramingState gTRKFramingState;
volatile u8* gTRKInputPendingPtr;

static const char packet_header[] = "TRK_Packet_Header \t    %ld bytes\n";
static const char read_memory[] = "TRK_CMD_ReadMemory     %ld bytes\n";
static const char write_memory[] = "TRK_CMD_WriteMemory    %ld bytes\n";
static const char connect[] = "TRK_CMD_Connect \t    %ld bytes\n";
static const char reply_ack[] = "TRK_CMD_ReplyAck\t    %ld bytes\n";
static const char read_registers[] = "TRK_CMD_ReadRegisters\t%ld bytes\n";
static const char free_buffer[] = "TestForPacket : FreeBuffer is  %ld\n";
static const char reading_payload[] = "Reading payload %ld bytes\n";
static const char invalid_header[] = "TestForPacket : Invalid size of packet hdr.size\n";
static const char invalid_packet[] = "TestForPacket : Invalid size of packet\n";
static const char packet_result[] = "TestForPacket returning %ld\n";

extern void MWTRACE(int level, const char* format, ...);
extern int TRKPollUART(void);
extern DSError TRKGetFreeBuffer(int* buffer_id, MessageBuffer** buffer);
extern MessageBuffer* TRKGetBuffer(int buffer_id);
extern void TRKSetBufferPosition(MessageBuffer* buffer, u32 position);
extern DSError TRKReadUARTN(void* data, u32 length);
extern DSError TRKAppendBuffer_ui8(MessageBuffer* buffer, const void* data, u32 length);
extern void TRKReleaseBuffer(int buffer_id);
extern void TRKConstructEvent(TRKEvent* event, int event_type);
extern DSError TRKPostEvent(TRKEvent* event);

MessageBufferID TRKTestForPacket(void)
{
    u8 payload[0x880];
    PacketHeader header;
    int buffer_id;
    MessageBuffer* buffer;
    MessageBufferID result;

    if (TRKPollUART() <= 0)
        return -1;
    result = TRKGetFreeBuffer(&buffer_id, &buffer);
    MWTRACE(4, free_buffer, result);
    TRKSetBufferPosition(buffer, 0);
    if (TRKReadUARTN(header.bytes, sizeof(header)) == 0) {
        int payload_size;

        TRKAppendBuffer_ui8(buffer, header.bytes, sizeof(header));
        payload_size = header.size - sizeof(header);
        result = buffer_id;
        if (payload_size > 0) {
            MWTRACE(1, reading_payload, payload_size);
            if (TRKReadUARTN(payload, header.size - sizeof(header)) == 0) {
                TRKAppendBuffer_ui8(buffer, payload, header.size);
            } else {
                MWTRACE(8, invalid_header);
                TRKReleaseBuffer(result);
                result = -1;
            }
        }
    } else {
        MWTRACE(8, invalid_packet);
        TRKReleaseBuffer(result);
        result = -1;
    }
    MWTRACE(1, packet_result, result);
    return result;
}

void TRKGetInput(void)
{
    MessageBufferID buffer_id = TRKTestForPacket();

    if (buffer_id != -1) {
        TRKEvent event;

        TRKGetBuffer(buffer_id);
        TRKConstructEvent(&event, 2);
        event.message_buffer_id = buffer_id;
        gTRKFramingState.message_buffer_id = -1;
        TRKPostEvent(&event);
    }
}

void TRKProcessInput(int buffer_id)
{
    TRKEvent event;

    TRKConstructEvent(&event, 2);
    event.message_buffer_id = buffer_id;
    gTRKFramingState.message_buffer_id = -1;
    TRKPostEvent(&event);
}

DSError TRKInitializeSerialHandler(void)
{
    gTRKFramingState.message_buffer_id = -1;
    gTRKFramingState.receive_state = 0;
    gTRKFramingState.is_escape = 0;
    MWTRACE(1, packet_header, 0x40);
    MWTRACE(1, read_memory, 0x40);
    MWTRACE(1, write_memory, 0x40);
    MWTRACE(1, connect, 0x40);
    MWTRACE(1, reply_ack, 0x40);
    MWTRACE(1, read_registers, 0x40);
    return 0;
}

DSError TRKTerminateSerialHandler(void)
{
    return 0;
}
