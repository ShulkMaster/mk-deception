#include "libmkparticle/rw_engine.h"
#include "runtime/cfile.h"

RwFileFunctions* RwOsGetFileInterface(void) {
    return &RwEngineInstance->fileFuncs;
}

static RwBool rwfexist(const RwChar* name) {
    FILE* file = RwEngineInstance->fileFuncs.rwfopen(name, "rb");
    RwBool exists = file != NULL;

    if (file != NULL) {
        RwEngineInstance->fileFuncs.rwfclose(file);
    }
    return exists;
}

RwBool _rwFileSystemOpen(void) {
    RwEngineInstance->fileFuncs.rwfexist = rwfexist;
    RwEngineInstance->fileFuncs.rwfopen = (rwFnFopen)fopen;
    RwEngineInstance->fileFuncs.rwfclose = (rwFnFclose)fclose;
    RwEngineInstance->fileFuncs.rwfread = (rwFnFread)fread;
    RwEngineInstance->fileFuncs.rwfwrite = (rwFnFwrite)fwrite;
    RwEngineInstance->fileFuncs.rwfgets = (rwFnFgets)fgets;
    RwEngineInstance->fileFuncs.rwfputs = (rwFnFputs)fputs;
    RwEngineInstance->fileFuncs.rwfeof = (rwFnFeof)feof;
    RwEngineInstance->fileFuncs.rwfseek = (rwFnFseek)fseek;
    RwEngineInstance->fileFuncs.rwfflush = (rwFnFflush)fflush;
    RwEngineInstance->fileFuncs.rwftell = (rwFnFtell)ftell;
    return TRUE;
}

void _rwFileSystemClose(void) {}
