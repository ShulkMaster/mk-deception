#ifndef MWSCREENENGINE_SCREENCLIENT_H
#define MWSCREENENGINE_SCREENCLIENT_H
class ScreenObject; class ScreenMgr; class ScreenAction; class ScreenSet; class Screen;
class ScreenRenderInfo; class ScreenResourceLibrary; class ScreenMatrixStack; class ScreenParams;
class ScreenClient {
public:
    ScreenClient();
    virtual void PrintObjectDepth(ScreenRenderInfo*, int, int);
    virtual ~ScreenClient();
    virtual void* Malloc(unsigned long, int, char*) = 0;
    virtual void Free(void*) = 0;
    virtual ScreenResourceLibrary* CreateResourceLibrary(ScreenResourceLibrary*) = 0;
    virtual void DestroyResourceLibrary(ScreenResourceLibrary*) = 0;
    virtual ScreenMatrixStack* CreateMatrixStack() = 0;
    virtual void DestroyMatrixStack(ScreenMatrixStack*) = 0;
    virtual void* CreateElement(ScreenMgr*, Screen*, ScreenObject*, void*) = 0;
    virtual ScreenObject* CreateInstance(ScreenMgr*, int, ScreenParams*) = 0;
    virtual int LoadScreenSet(ScreenSet*) = 0;
    virtual int DoneLoadingSet(ScreenSet*);
    virtual int LoadScreen(ScreenSet*, Screen*, unsigned int) = 0;
    virtual void UnloadScreen(Screen*) = 0;
    virtual void UnloadScreenSet(int) = 0;
    virtual void PreRender() = 0;
    virtual void PostRender() = 0;
    virtual void SetCurrent(ScreenSet*) = 0;
    virtual void Reset() = 0;
    virtual void SetRootTransformation(ScreenMatrixStack*) = 0;
    virtual void HandleEvent(ScreenObject*, int, int);
    virtual void HandleAction(ScreenMgr*, const ScreenAction*, int);
    virtual ScreenAction* CreateAction(int);
    virtual int IsControllerActive(int);
    virtual void ReportError(char*, char*, int) = 0;
    virtual void PreloadData(int);
    virtual int IsPreloadDataDone(int);
};
#endif
