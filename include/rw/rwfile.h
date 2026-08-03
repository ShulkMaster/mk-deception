#ifndef RW_RWFILE_H
#define RW_RWFILE_H

/* RenderWare installs operations with several signatures through this table. */
typedef int (*RwFileFunction)();

typedef struct RwFileInterface {
    RwFileFunction open;
    RwFileFunction close;
    RwFileFunction read;
    RwFileFunction write;
    RwFileFunction gets;
    RwFileFunction puts;
    RwFileFunction eof;
    RwFileFunction seek;
    RwFileFunction flush;
    RwFileFunction tell;
    RwFileFunction exists;
} RwFileInterface;

RwFileInterface* RwOsGetFileInterface(void);

#endif
