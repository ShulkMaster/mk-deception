#ifndef MWSCREENENGINE_SCREENMGR_H
#define MWSCREENENGINE_SCREENMGR_H

#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenSet.h"
#include "mwScreenEngine/ScreenActionStack.h"

class ScreenClient;
class ScreenAction;

typedef unsigned int (*ScreenRegisterFn)(const ScreenAction* action);

struct ScreenRegisterEntry {
    ScreenRegisterFn fn; /* +0x00 */
    unsigned int id; /* +0x04 */
};
class ScreenMgr {
public:
    ScreenMgr();

    /* Retail vtbl: Init, Dispose, dtor, LoadCompleted */
    virtual void Init(ScreenClient* client);
    virtual void Dispose(unsigned int flags);
    virtual ~ScreenMgr();
    virtual void LoadCompleted(ScreenSet* set);

    void Reset();
    Screen* GetActiveScreen();
    ScreenActionStack* GetActionStack() { return &m_actionStack; }
    /* Returns 1 on accept (may still be loading); 0 if InitBranchPath fails. */
    int LoadScreen(char* path, unsigned int flags);

    int InitBranchPath();
    int UpdateBranchPath(char* path);
    int FindScreen(char* path, Screen** outScreen);
    ScreenSet* FindParent(ScreenSet* set, char** parts, int nParts, int& depth);
    int SplitPath(char* path, const char* delim, char** outParts, int maxParts);

    int GetScreenIndex(Screen* screen);
    int RemoveScreen(Screen* screen);
    void RemoveTopScreen();
    void RemoveScreens(ScreenSet* set);
    void AppendScreen(Screen* screen);
    void InsertScreen(Screen* screen, int index);
    void CloseScreen(Screen* screen);
    Screen* CloseTopScreen();
    void OpenScreen(Screen* screen);
    void DisposeSet(ScreenSet* set, unsigned int flags);

    void FireEvent(int event, int arg, unsigned int force);
    void BroadcastEvent(int event, int activeOnly, int arg);
    void Idle(int dt);
    void UpdateAnimations(int dt);
    void Render();

    void SetConfirmUser(int index, unsigned int value, int checkCount);
    void ResetStagesTo(int value);
    int GetStage(int index);
    void SetStage(int index, int value);
    void RegisterActionHandler(unsigned int id, ScreenRegisterFn fn);
    int ProcessRegisterActions(const ScreenAction* action);

    /* +0x00 vptr */
    int m_exitStagePending; /* +0x04 -- Exit special (0x3EE) latch */
    int m_exitStageValue; /* +0x08 -- stage value written with latch */
    unsigned int m_pendingOpen; /* +0x0c */
    unsigned int m_suppressDraw; /* +0x10 */
    unsigned int m_eventsEnabled; /* +0x14 */
    int m_branchDepth; /* +0x18 */
    ScreenSet* m_branch[16]; /* +0x1c */
    char m_pathBuf[0x80]; /* +0x5c */
    ScreenSet* m_rootSet; /* +0xdc */
    ScreenSet* m_currentSet; /* +0xe0 */
    char m_loadPath[0x80]; /* +0xe4 */
    char m_screenName[0x40]; /* +0x164 */
    int m_activeCount; /* +0x1a4 */
    Screen* m_stack[16]; /* +0x1a8 */
    int m_unk1e8; /* +0x1e8 */
    unsigned char m_pad1ec[0x40]; /* +0x1ec .. +0x22b */
    ScreenActionStack m_actionStack; /* +0x22c */
    unsigned int m_confirm[4]; /* +0x248 -- confirm + GetStage/SetStage words */
    int m_registerCount; /* +0x258 */
    ScreenRegisterEntry* m_registerTable; /* +0x25c */
    unsigned int m_eventLatch; /* +0x260 */
}; /* sizeof == 0x264 -- matches Glue BSS screen_manager */

void __dt__9ScreenMgrFv(void* self, short del);
void* __ct__9ScreenMgrFv(void* self);
#endif
