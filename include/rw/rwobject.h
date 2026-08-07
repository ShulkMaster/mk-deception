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

typedef enum RwStreamType {
    rwSTREAMFILE = 1,
    rwSTREAMFILENAME = 2,
    rwSTREAMMEMORY = 3,
    rwSTREAMCUSTOM = 4
} RwStreamType;

typedef enum RwStreamAccessType {
    rwSTREAMREAD = 1,
    rwSTREAMWRITE = 2,
    rwSTREAMAPPEND = 3
} RwStreamAccessType;

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

/** Stock RenderWare stream object. Retail size: 0x24 bytes. */
typedef struct RwStream {
    RwStreamType type;              /**< Retail offset 0x00. */
    RwStreamAccessType accessType;  /**< Retail offset 0x04. */
    unsigned int reserved;          /**< Retail offset 0x08. */
    union {
        struct { void* file; } file;
        struct {
            unsigned int position;
            unsigned int length;
            unsigned char* start;
        } memory;
        RwStreamCustom custom;
    } data;                         /**< Retail offset 0x0C. */
    int owned;                      /**< Retail offset 0x20. */
} RwStream;

#endif
