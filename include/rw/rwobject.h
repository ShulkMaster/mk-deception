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

/** Partial RenderWare stream view; complete object extent is unknown. */
typedef struct RwStream {
    char pad00[0x0C];          /**< Retail offsets 0x00-0x0B; fields unknown. */
    unsigned int bufferPosition; /**< Retail offset 0x0C. */
    char pad10[0x04];          /**< Retail offsets 0x10-0x13; fields unknown. */
    unsigned char* data;       /**< Retail offset 0x14. */
} RwStream;

#endif
