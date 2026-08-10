#ifndef RW_RWOBJECT_H
#define RW_RWOBJECT_H

/** Stock RenderWare object header. Retail layout: 0x08 bytes. */
typedef struct RwObject {
    unsigned char type;         /**< Retail offset 0x00. */
    unsigned char subType;      /**< Retail offset 0x01. */
    unsigned char flags;        /**< Retail offset 0x02. */
    unsigned char privateFlags; /**< Retail offset 0x03. */
    void* parent;               /**< Retail offset 0x04; commonly an `RwFrame*`. */
} RwObject;

typedef int RwStreamType;
typedef int RwStreamAccessType;
typedef void (*RwStreamCloseCallBack)(void* data);
typedef unsigned int (*RwStreamReadCallBack)(void* data, void* buffer,
                                             unsigned int length);
typedef int (*RwStreamWriteCallBack)(void* data, const void* buffer,
                                     unsigned int length);
typedef int (*RwStreamSkipCallBack)(void* data, unsigned int offset);

typedef struct RwStreamCustom {
    RwStreamCloseCallBack close;
    RwStreamReadCallBack read;
    RwStreamWriteCallBack write;
    RwStreamSkipCallBack skip;
    void* data;
} RwStreamCustom;

typedef struct RwStream {
    RwStreamType type;
    RwStreamAccessType accessType;
    unsigned int field_08;
    union {
        struct {
            void* file;
        } file;
        struct {
            unsigned int position;
            unsigned int length;
            unsigned char* start;
        } memory;
        RwStreamCustom custom;
    } data;
    int owned;
} RwStream;

#endif
