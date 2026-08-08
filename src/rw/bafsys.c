#include "libmkparticle/rw_engine.h"
#include "runtime/cfile.h"

RwFileFunctions* RwOsGetFileInterface(void) {
    return &RwEngineInstance->fileFuncs;
}

/* Near miss: retail lowers the pointer-to-boolean conversion with
 * subic/subfe; this compiler expression emits the equivalent neg/or pair. */
static RwBool rwfexist(const RwChar* name) {
    void* file;
    RwBool exists;

    file = RwEngineInstance->fileFuncs.rwfopen(name, "rb");
    exists = file != 0;

    if (file != 0) {
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
    return 1;
}

void _rwFileSystemClose(void) {}
