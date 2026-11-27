#ifndef MWSCREENENGINE_SCREEN_TEXT_H
#define MWSCREENENGINE_SCREEN_TEXT_H

#include "mwScreenEngine/ScreenNode.h"

typedef struct PfxFontSlot PfxFontSlot;

typedef struct StringObj StringObj;
struct _MKV3D;
class ScreenText;

#ifdef __cplusplus
class ScreenAnimControl;
#endif

/* C view matching SETextElement_t (ScreenObject.h). */
struct SEText_t {
    unsigned int typeTag; /* +0x00 'TEXT' */
    int pad04; /* +0x04 */
    ScreenText* liveObject; /* +0x08 -- after instancing */
    unsigned int unk0c; /* +0x0C */
    char* string0; /* +0x10 -- after Patch */
    char* string1; /* +0x14 -- font name after Patch */
    float posX; /* +0x18 */
    float posY; /* +0x1C -- screen Y via (480 - posY) */
    float unk20; /* +0x20 */
    float wrapW; /* +0x24 */
    float yOff; /* +0x28 */
    unsigned char color[4]; /* +0x2C */
    unsigned char halign; /* +0x30 */
    unsigned char valign; /* +0x31 */
};

class ScreenText : public ScreenNode {
public:
    ScreenText(SEText_t* se);
    virtual ~ScreenText();
    virtual void Render(ScreenRenderInfo* info);
    virtual void SetVisible(unsigned int visible);
    virtual unsigned int IsVisible();
    virtual void ProcessEngineEvent(ScreenMgr* mgr, int event);
    virtual void Close();

    int GetStringLen();
    void GetPosition(_MKV3D* pos);
    int GetWidth();
    void SetColor(ScreenColor_t* color);
    void GetColor(ScreenColor_t* color);
    void SetString(char* string);
    void ChangeCase(int toUpper);
    int GetLinesAvailable();
    int InstanceText();
    int* GetStartArray(char* str, int& outCount);
    void SetVertsToBoundingBox(_MKV3D* verts);

    SEText_t* seData; /* +0x10 */
    StringObj* stringObj; /* +0x14 */
    int stringObjInstance; /* +0x18 */
    PfxFontSlot* font; /* +0x1C */
    char* string; /* +0x20 */
    virtual void SetComponent(ScreenAnimControl* ctrl, float* values, int);
}; /* 0x24 */

enum {
    kScreenTextStringOid = 0x900C,
    kScreenTextEvtRebuildA = 0x407,
    kScreenTextEvtNoOp = 0x408, /* cascade hole -- ScreenText returns */
    kScreenTextEvtRebuildB = 0x409
};

#endif
