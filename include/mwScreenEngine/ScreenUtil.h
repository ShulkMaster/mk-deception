#ifndef MWSCREENENGINE_SCREENUTIL_H
#define MWSCREENENGINE_SCREENUTIL_H
class ScreenClient; class ScreenMgr; class Screen; class ScreenSet; class ScreenObject;
class ScreenAction; class ScreenMatrixStack; class ScreenResourceLibrary;
class ScreenUtil {
public:
    static void ReportError(char*, char*, int);
    static void SetScreenClient(ScreenClient*);
    static ScreenClient* GetScreenClient();
    static void* Malloc(unsigned long, int, char*);
    static void Free(void*);
    static ScreenMatrixStack* CreateMatrixStack();
    static void DestroyMatrixStack(ScreenMatrixStack*);
    static void* CreateElement(ScreenMgr*, Screen*, ScreenObject*, void*);
    static ScreenResourceLibrary* CreateResourceLibrary(ScreenResourceLibrary*);
    static void DestroyResourceLibrary(ScreenResourceLibrary*);
    static void PreRender(); static void PostRender(); static void SetCurrent(ScreenSet*);
    static void Reset(); static int ReadHexInt(char*); static void UnloadScreen(Screen*);
    static void PreloadData(int); static int IsPreloadDataDone(int);
    static int DoneLoadingSet(ScreenSet*); static int LoadScreenSet(ScreenSet*);
    static void UnloadScreenSet(int); static void SetRootTransformation(ScreenMatrixStack*);
    static void HandleEvent(ScreenObject*, int, int);
    static void HandleAction(ScreenMgr*, const ScreenAction*, int);
    static ScreenAction* CreateAction(int);
    static ScreenClient* m_pClient;
};
#endif
