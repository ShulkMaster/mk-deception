#ifndef RW_RWPLCORE_H
#define RW_RWPLCORE_H

typedef int RwInt32;
typedef unsigned int RwUInt32;
typedef unsigned short RwUInt16;
typedef signed short RwInt16;
typedef unsigned char RwUInt8;
typedef int RwBool;
typedef char RwChar;
typedef float RwReal;
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
    volatile RwUInt32 nUInt;
} RwSplitBits;
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
typedef RwBool (*RwPluginDataChunkAlwaysCallBack)(void*, RwInt32, RwInt32);
typedef RwBool (*RwPluginDataChunkRightsCallBack)(void*, RwInt32, RwInt32,
                                                  RwUInt32);
typedef void* (*RwPluginErrorStrCallBack)(void*);
struct RwPluginRegEntry {
    RwInt32 offset;
    RwInt32 size;
    RwUInt32 pluginID;
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
RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry*, RwInt32, RwUInt32,
                                   RwPluginObjectConstructor, RwPluginObjectDestructor,
                                   RwPluginObjectCopy);
RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry*, RwUInt32);
const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry*,
                                                    void*);
const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry*,
                                                      void*);
RwInt32 _rwPluginRegistryAddPluginStream(
    RwPluginRegistry*, RwUInt32, RwPluginDataChunkReadCallBack,
    RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack);
RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(
    RwPluginRegistry*, RwUInt32, RwPluginDataChunkAlwaysCallBack);
RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(
    RwPluginRegistry*, RwUInt32, RwPluginDataChunkRightsCallBack);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(
    const RwPluginRegistry*, RwStream*, void*);
const RwPluginRegistry* _rwPluginRegistryInvokeRights(
    const RwPluginRegistry*, RwUInt32, void*, RwUInt32);
RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry*, const void*);
const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(
    const RwPluginRegistry*, RwStream*, const void*);
const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(
    const RwPluginRegistry*, RwStream*);
RwInt32 RwEngineRegisterPlugin(RwInt32, RwUInt32,
                               RwPluginObjectConstructor,
                               RwPluginObjectDestructor);
RwBool _rwpathisabsolute(const RwChar*);
#endif
