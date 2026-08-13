#ifndef RW_RWDEVICE_H
#define RW_RWDEVICE_H

typedef struct RwMemoryFunctions RwMemoryFunctions;

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

#endif
