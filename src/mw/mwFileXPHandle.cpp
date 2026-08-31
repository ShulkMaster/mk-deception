#include "mw/mwFile.h"
#include "runtime/cstring.h"

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(unsigned long size, mwTargetMemAlign alignment,
                          const char* name);
    static void deallocate(void* memory);
};

template <class T>
bool mwFileMemAlloc(T*& memory, unsigned long size,
                    mwTargetMemAlign alignment, const char* name);

class mwFileBuffer {
public:
    mwFileBuffer();
    ~mwFileBuffer();

    unsigned long readInRange(unsigned char*& destination,
                              unsigned long& length,
                              unsigned long long& position);
    void reset(unsigned long long position, unsigned long length);
    void shutdown();
    int initialize(unsigned long size, mwTargetMemAlign alignment);

private:
    unsigned long long file_position;
    unsigned long capacity;
    unsigned char* buffer;
    unsigned char* buffer_end;
    bool dirty;
};

typedef char mwFileBufferSizeCheck[sizeof(mwFileBuffer) == 0x18 ? 1 : -1];

extern void _mwFileNoOp(...);

static const char stringBase0[] =
    "mwFile: WARNING - destroying unflushed file buffer\n\0"
    "File buffer";

unsigned long mwFileBuffer::readInRange(unsigned char*& destination,
                                        unsigned long& length,
                                        unsigned long long& position)
{
    unsigned long bytes_read = 0;

    if (buffer != buffer_end) {
        unsigned long long buffered_end =
            file_position + (unsigned long)(buffer_end - buffer);

        if (position >= file_position && position < buffered_end) {
            unsigned long buffered_length =
                (unsigned long)(buffered_end - position);
            if (buffered_length > length) {
                buffered_length = length;
            }

            memcpy(destination,
                   buffer + (unsigned long)(position - file_position),
                   buffered_length);
            position += buffered_length;
            length -= buffered_length;
            destination += buffered_length;
            bytes_read = buffered_length;
        }

        if (length != 0 && position + length >= file_position &&
            position + length < buffered_end) {
            unsigned long buffered_length =
                (unsigned long)(position + length - file_position);
            memcpy(destination + length - buffered_length, buffer,
                   buffered_length);
            bytes_read += buffered_length;
            length -= buffered_length;
        }
    }

    return bytes_read;
}

void mwFileBuffer::reset(unsigned long long position, unsigned long length)
{
    buffer_end = buffer + length;
    dirty = false;
    file_position = position;
}

void mwFileBuffer::shutdown()
{
    if (dirty) {
        _mwFileNoOp(&stringBase0[0]);
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

int mwFileBuffer::initialize(unsigned long size, mwTargetMemAlign alignment)
{
    if (buffer != 0) {
        return 0;
    }

    capacity = size;
    file_position = 0;
    if (!mwFileMemAlloc(buffer, size, alignment, &stringBase0[52])) {
        return -10;
    }
    if (buffer == 0) {
        return -10;
    }

    buffer_end = buffer;
    dirty = false;
    return 0;
}

mwFileBuffer::~mwFileBuffer()
{
    shutdown();
}

mwFileBuffer::mwFileBuffer()
    : file_position(~0ULL), capacity(0), buffer(0), buffer_end(0), dirty(false)
{
}

template <class T>
bool mwFileMemAlloc(T*& memory, unsigned long size,
                    mwTargetMemAlign alignment, const char* name)
{
    memory = static_cast<T*>(
        mwFileMultithreadedMemTraits::allocate(size, alignment, name));
    return memory != 0;
}
