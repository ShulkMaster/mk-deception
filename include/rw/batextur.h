#ifndef RW_BATEXTUR_H
#define RW_BATEXTUR_H

#include "rw/rwcore_types.h"

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

#endif
