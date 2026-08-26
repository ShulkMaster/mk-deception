typedef unsigned char u8;
typedef unsigned long u32;
typedef int DSError;

typedef struct MessageBuffer {
    u32 unknown;
    int is_in_use;
    u32 length;
    u32 position;
    u8 data[0x880];
} MessageBuffer;

extern DSError TRKWriteUARTN(const void* data, u32 length);
extern void MWTRACE(int level, const char* format, ...);

DSError TRKMessageSend(MessageBuffer* message)
{
    DSError write_error = TRKWriteUARTN(message->data, message->length);
    MWTRACE(1, "MessageSend : cc_write returned %ld\n", write_error);
    return 0;
}
