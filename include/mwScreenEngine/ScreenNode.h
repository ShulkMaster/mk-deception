#ifndef MWSCREENENGINE_SCREENNODE_H
#define MWSCREENENGINE_SCREENNODE_H
class ScreenMgr; class ScreenAction; class ScreenMatrixStack; class ScreenAnimControl;
class ScreenRenderInfo;
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
    virtual void ProcessEngineEvent(ScreenMgr*, int);
    virtual int HandleAction(ScreenMgr*, const ScreenAction*);
    virtual void SetMatrixStack(ScreenMatrixStack*);
    virtual int GetNumNodes() const;
    virtual int NeedIdleProcessing();
    virtual void Close();
    void* operator new(unsigned long);
    void operator delete(void*);
    unsigned int m_flags;
    unsigned int typeTag;
    ScreenNode* next;
};
#endif
