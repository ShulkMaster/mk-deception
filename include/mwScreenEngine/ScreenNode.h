#ifndef MWSCREENENGINE_SCREENNODE_H
#define MWSCREENENGINE_SCREENNODE_H
#include "dolphin/types.h"
class ScreenMgr; class ScreenAction; class ScreenMatrixStack; class ScreenAnimControl;
class ScreenRenderInfo;
struct ScreenColor_t {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};
class ScreenNode {
public:
    ScreenNode();
    virtual ~ScreenNode();
    virtual void Init();
    virtual void Dispose();
    virtual void Render(ScreenRenderInfo*);
    virtual void SetComponent(ScreenAnimControl*, float*, int);
    virtual void SetVisible(unsigned int) = 0;
    virtual unsigned int IsVisible() = 0;
    virtual void ProcessEngineEvent(ScreenMgr*, int) {}
    virtual int HandleAction(ScreenMgr*, const ScreenAction*) { return 0; }
    virtual void SetMatrixStack(ScreenMatrixStack*) {}
    virtual int GetNumNodes() const { return 0; }
    virtual int NeedIdleProcessing() { return 0; }
    virtual void Close() {}
    void* operator new(size_t);
    void operator delete(void*);
    unsigned int m_flags;
    unsigned int typeTag;
    ScreenNode* next;
};
#endif
