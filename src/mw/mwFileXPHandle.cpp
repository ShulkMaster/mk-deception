#include "mw/mwFile.h"
#include "runtime/cstring.h"

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(size_t size, mwTargetMemAlign alignment,
                          const char* name);
    static void deallocate(void* memory);
};

template <class T>
bool mwFileMemAlloc(T*& memory, size_t size,
                    mwTargetMemAlign alignment, const char* name);

class mwFileBuffer {
public:
    mwFileBuffer();
    ~mwFileBuffer();

    u32 readInRange(unsigned char*& destination,
                              u32& length,
                              unsigned long long& position);
    void reset(unsigned long long position, u32 length);
    void shutdown();
    int initialize(u32 size, mwTargetMemAlign alignment);

private:
    unsigned long long file_position;
    u32 capacity;
    unsigned char* buffer;
    unsigned char* buffer_end;
    bool dirty;
};

typedef char mwFileBufferSizeCheck[sizeof(mwFileBuffer) == 0x18 ? 1 : -1];

extern void _mwFileNoOp(...);

u32 mwFileBuffer::readInRange(unsigned char*& destination,
                                        u32& length,
                                        unsigned long long& position)
{
    u32 bytes_read = 0;

    if (buffer != buffer_end) {
        unsigned long long buffered_end =
            file_position + (u32)(buffer_end - buffer);

        if (position >= file_position && position < buffered_end) {
            u32 buffered_length =
                buffered_end - position;
            if (buffered_length > length) {
                buffered_length = length;
            }

            memcpy(destination,
                   buffer + (u32)(position - file_position),
                   buffered_length);
            position += buffered_length;
            length -= buffered_length;
            destination += buffered_length;
            bytes_read = buffered_length;
        }

        if (length != 0 && position + length >= file_position &&
            position + length < buffered_end) {
            u32 buffered_length =
                position + length - file_position;
            memcpy(destination + (length - buffered_length), buffer,
                   buffered_length);
            bytes_read += buffered_length;
            length -= buffered_length;
        }
    }

    return bytes_read;
}

void mwFileBuffer::reset(unsigned long long position, u32 length)
{
    buffer_end = buffer + length;
    dirty = false;
    file_position = position;
}

void mwFileBuffer::shutdown()
{
    if (dirty) {
        _mwFileNoOp("mwFile: WARNING - destroying unflushed file buffer\n");
    }

    if (buffer != 0) {
        mwFileMultithreadedMemTraits::deallocate(buffer);
        buffer = 0;
        buffer_end = 0;
        capacity = 0;
        file_position = 0;
        dirty = false;
    }
}

int mwFileBuffer::initialize(u32 size, mwTargetMemAlign alignment)
{
    if (buffer != 0) {
        return 0;
    }

    capacity = size;
    file_position = 0;
    if (!mwFileMemAlloc(buffer, size, alignment, "File buffer")) {
        return -10;
    }
    if (buffer == 0) {
        return -10;
    }

    buffer_end = buffer;
    dirty = false;
    return 0;
}

#pragma push
mwFileBuffer::~mwFileBuffer()
{
    shutdown();
}
#pragma pop

mwFileBuffer::mwFileBuffer()
    : file_position(~0ULL), capacity(0), buffer(0), buffer_end(0), dirty(false)
{
}

template <class T>
bool mwFileMemAlloc(T*& memory, size_t size,
                    mwTargetMemAlign alignment, const char* name)
{
    memory = static_cast<T*>(
        mwFileMultithreadedMemTraits::allocate(size, alignment, name));
    return memory != 0;
}
