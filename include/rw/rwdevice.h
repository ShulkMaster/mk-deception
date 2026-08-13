#ifndef RW_RWDEVICE_H
#define RW_RWDEVICE_H

typedef struct RwMemoryFunctions {
    void* (*alloc)(unsigned long size, unsigned int hint);
    void (*free)(void* memory);
    void* (*realloc)(void* memory, unsigned long size, unsigned int hint);
    void* (*calloc)(unsigned long count, unsigned long size,
                    unsigned int hint);
} RwMemoryFunctions;

typedef struct RwVideoMode {
    int width;
    int height;
    int depth;
    int flags;
    int refreshRate;
    int format;
} RwVideoMode;

typedef struct RwEngineOpenParams {
    void* displayID;
} RwEngineOpenParams;

int RwEngineGetCurrentVideoMode(void);
RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeInfo, int mode);
int RwEngineInit(const RwMemoryFunctions* memoryFunctions,
                 unsigned int initFlags, unsigned int resArenaSize);
int RwEngineOpen(RwEngineOpenParams* params);
int RwEngineClose(void);
int RwEngineStart(void);
int RwEngineTerm(void);
int _rwDeviceRegisterPlugin(void);

#endif
