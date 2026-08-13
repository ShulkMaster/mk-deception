#ifndef RW_RWPLCORE_H
#define RW_RWPLCORE_H

typedef union RwSplitBits {
    float nReal;
    volatile int nInt;
    volatile unsigned int nUInt;
} RwSplitBits;
#define NULL 0
typedef struct RwModuleInfo { int globalsOffset; int numInstances; } RwModuleInfo;
typedef struct RwV2d { float x; float y; } RwV2d;
typedef struct RwV3d { float x; float y; float z; } RwV3d;
typedef struct RwBBox { RwV3d sup; RwV3d inf; } RwBBox;
typedef struct RwSphere {
    RwV3d center;
    float radius;
} RwSphere;
typedef struct RwError {
    int pluginID;
    int errorCode;
} RwError;
typedef struct RwPluginRegEntry RwPluginRegEntry;
typedef struct RwStream RwStream;
typedef struct RwPluginRegistry {
    int sizeOfStruct;
    int origSizeOfStruct;
    int maxSizeOfStruct;
    int staticAlloc;
    RwPluginRegEntry* firstRegEntry;
    RwPluginRegEntry* lastRegEntry;
} RwPluginRegistry;
typedef void* (*RwPluginObjectConstructor)(void*, int, int);
typedef void* (*RwPluginObjectDestructor)(void*, int, int);
typedef void* (*RwPluginObjectCopy)(void*, const void*, int, int);
typedef RwStream* (*RwPluginDataChunkReadCallBack)(RwStream*, int, void*, int,
                                                   int);
typedef RwStream* (*RwPluginDataChunkWriteCallBack)(RwStream*, int, const void*, int,
                                                    int);
typedef int (*RwPluginDataChunkGetSizeCallBack)(const void*, int, int);
typedef int (*RwPluginDataChunkAlwaysCallBack)(void*, int, int);
typedef int (*RwPluginDataChunkRightsCallBack)(void*, int, int,
                                                  unsigned int);
typedef void* (*RwPluginErrorStrCallBack)(void*);
struct RwPluginRegEntry {
    int offset;
    int size;
    unsigned int pluginID;
    RwPluginDataChunkReadCallBack readCB;
    RwPluginDataChunkWriteCallBack writeCB;
    RwPluginDataChunkGetSizeCallBack getSizeCB;
    RwPluginDataChunkAlwaysCallBack alwaysCB;
    RwPluginDataChunkRightsCallBack rightsCB;
    RwPluginObjectConstructor constructCB;
    RwPluginObjectDestructor destructCB;
    RwPluginObjectCopy copyCB;
    RwPluginErrorStrCallBack errStrCB;
    RwPluginRegEntry* nextRegEntry;
    RwPluginRegEntry* prevRegEntry;
    RwPluginRegistry* parentRegistry;
};
int _rwPluginRegistryAddPlugin(RwPluginRegistry*, int, unsigned int,
                                   RwPluginObjectConstructor, RwPluginObjectDestructor,
                                   RwPluginObjectCopy);
int _rwPluginRegistryGetPluginOffset(const RwPluginRegistry*, unsigned int);
const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry*,
                                                    void*);
const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry*,
                                                      void*);
int _rwPluginRegistryAddPluginStream(
    RwPluginRegistry*, unsigned int, RwPluginDataChunkReadCallBack,
    RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack);
int _rwPluginRegistryAddPlgnStrmlwysCB(
    RwPluginRegistry*, unsigned int, RwPluginDataChunkAlwaysCallBack);
int _rwPluginRegistryAddPlgnStrmRightsCB(
    RwPluginRegistry*, unsigned int, RwPluginDataChunkRightsCallBack);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(
    const RwPluginRegistry*, RwStream*, void*);
const RwPluginRegistry* _rwPluginRegistryInvokeRights(
    const RwPluginRegistry*, unsigned int, void*, unsigned int);
int _rwPluginRegistryGetSize(const RwPluginRegistry*, const void*);
const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(
    const RwPluginRegistry*, RwStream*, const void*);
const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(
    const RwPluginRegistry*, RwStream*);
int RwEngineRegisterPlugin(int, unsigned int,
                               RwPluginObjectConstructor,
                               RwPluginObjectDestructor);
int RwEngineGetPluginOffset(unsigned int pluginID);
int _rwPluginRegistryOpen(void);
int _rwPluginRegistryClose(void);
int _rwpathisabsolute(const char*);
#endif
