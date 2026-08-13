#ifndef RW_BATEXTUR_H
#define RW_BATEXTUR_H

#include "rw/rwcore_types.h"

typedef RwTexture* (*RwTextureFindCallBack)(const char* name);
typedef RwTexture* (*RwTextureReadCallBack)(const char* name,
                                            const char* maskName);
typedef RwRaster* (*RwTextureMipmapGenerationCallBack)(RwRaster* raster,
                                                       RwImage* image);
typedef int (*RwTextureMipmapNameCallBack)(char* name, char* maskName,
                                              unsigned char level, int format);

RwTexDictionary* RwTexDictionaryGetCurrent(void);
void RwTexDictionarySetCurrent(RwTexDictionary* dictionary);
RwTexDictionary* RwTexDictionaryCreate(void);
int RwTexDictionaryDestroy(RwTexDictionary* dictionary);
RwTexture* RwTextureSetMaskName(RwTexture* texture, const char* maskName);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dictionary,
                                     RwTexture* texture);
RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dictionary,
                                            const char* name);
RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data);
RwTexture* RwTextureStreamRead(RwStream* stream);
unsigned int RwTextureStreamGetSize(const RwTexture* texture);
const RwTexture* RwTextureStreamWrite(const RwTexture* texture,
                                      RwStream* stream);
RwTexture* RwTextureRead(const char* name, const char* maskName);
int RwTextureSetMipmapping(int enable);
int RwTextureGetMipmapping(void);
int RwTextureSetAutoMipmapping(int enable);
int RwTextureGetAutoMipmapping(void);
int RwTextureSetFindCallBack(RwTextureFindCallBack callback);
int RwTextureSetReadCallBack(RwTextureReadCallBack callback);
int RwTextureSetMipmapGenerationCallBack(
    RwTextureMipmapGenerationCallBack callback);
int RwTextureSetMipmapNameCallBack(RwTextureMipmapNameCallBack callback);
int RwTextureGenerateMipmapName(char* name, char* maskName, unsigned char level,
                                   int format);
int RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image);
int RwTextureRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
void* _rwTextureOpen(void* instance, int offset, int size);
void* _rwTextureClose(void* instance, int offset, int size);

#endif
