#include "rw/rwengine.h"
#include "rw/rwfilesystem.h"
#include "runtime/cfile.h"

RwFileFunctions* RwOsGetFileInterface(void) {
    return &RwEngineInstance->fileFuncs;
}


static int rwfexist(const char* name) {
    FILE* file;
    int exists;

    file = RwEngineInstance->fileFuncs.open(name, "rb");
    /* Preserve the open result after closing a successfully opened file. */
    exists = file != 0;

    if (file != 0) {
        RwEngineInstance->fileFuncs.close(file);
    }
    return exists;
}

int _rwFileSystemOpen(void) {
    RwEngineInstance->fileFuncs.exists = rwfexist;
    RwEngineInstance->fileFuncs.open = (RwFileOpenCall)fopen;
    RwEngineInstance->fileFuncs.close = (RwFileCloseCall)fclose;
    RwEngineInstance->fileFuncs.read = (RwFileReadCall)fread;
    RwEngineInstance->fileFuncs.write = (RwFileWriteCall)fwrite;
    RwEngineInstance->fileFuncs.gets = (RwFileGetsCall)fgets;
    RwEngineInstance->fileFuncs.puts = (RwFilePutsCall)fputs;
    RwEngineInstance->fileFuncs.eof = (RwFileEofCall)feof;
    RwEngineInstance->fileFuncs.seek = (RwFileSeekCall)fseek;
    RwEngineInstance->fileFuncs.flush = (RwFileFlushCall)fflush;
    RwEngineInstance->fileFuncs.tell = (RwFileTellCall)ftell;
    return 1;
}

void _rwFileSystemClose(void) {}
