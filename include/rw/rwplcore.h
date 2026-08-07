#ifndef RW_RWPLCORE_H
#define RW_RWPLCORE_H

typedef int RwInt32;
typedef unsigned int RwUInt32;
typedef unsigned short RwUInt16;
typedef int RwBool;
typedef char RwChar;
typedef float RwReal;
#define TRUE 1
#define FALSE 0
#define NULL 0

typedef struct RwModuleInfo { RwInt32 globalsOffset; RwInt32 numInstances; } RwModuleInfo;
typedef struct RwV3d { RwReal x; RwReal y; RwReal z; } RwV3d;
typedef struct RwBBox { RwV3d sup; RwV3d inf; } RwBBox;
typedef struct RwError {
    RwInt32 pluginID;
    RwInt32 errorCode;
} RwError;
typedef struct RwPluginRegEntry RwPluginRegEntry;
typedef struct RwStream RwStream;
typedef struct RwPluginRegistry {
    RwInt32 sizeOfStruct;
    RwInt32 origSizeOfStruct;
    RwInt32 maxSizeOfStruct;
    RwInt32 staticAlloc;
    RwPluginRegEntry* firstRegEntry;
    RwPluginRegEntry* lastRegEntry;
} RwPluginRegistry;
typedef void* (*RwPluginObjectConstructor)(void*, RwInt32, RwInt32);
typedef void* (*RwPluginObjectDestructor)(void*, RwInt32, RwInt32);
typedef void* (*RwPluginObjectCopy)(void*, const void*, RwInt32, RwInt32);
typedef RwStream* (*RwPluginDataChunkReadCallBack)(RwStream*, RwInt32, void*, RwInt32,
                                                   RwInt32);
typedef RwStream* (*RwPluginDataChunkWriteCallBack)(RwStream*, RwInt32, const void*, RwInt32,
                                                    RwInt32);
typedef RwInt32 (*RwPluginDataChunkGetSizeCallBack)(const void*, RwInt32, RwInt32);
RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry*, RwInt32, RwUInt32,
                                   RwPluginObjectConstructor, RwPluginObjectDestructor,
                                   RwPluginObjectCopy);
RwInt32 RwEngineRegisterPlugin(RwInt32, RwUInt32,
                               RwPluginObjectConstructor,
                               RwPluginObjectDestructor);
void* _rpSectorOpen(void*, RwInt32, RwInt32);
void* _rpSectorClose(void*, RwInt32, RwInt32);
RwInt32 RpWorldSectorRegisterPlugin(RwInt32, RwUInt32, RwPluginObjectConstructor,
                                    RwPluginObjectDestructor, RwPluginObjectCopy);
RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32, RwPluginDataChunkReadCallBack,
                                          RwPluginDataChunkWriteCallBack,
                                          RwPluginDataChunkGetSizeCallBack);
RwBBox* RwBBoxCalculate(RwBBox*, const RwV3d*, RwInt32);
RwBool _rwpathisabsolute(const RwChar*);
#endif
