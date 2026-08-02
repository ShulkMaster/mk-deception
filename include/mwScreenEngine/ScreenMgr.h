#ifndef MWSCREENENGINE_SCREENMGR_H
#define MWSCREENENGINE_SCREENMGR_H

#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenSet.h"
#include "mwScreenEngine/ScreenActionStack.h"

class ScreenClient;
class ScreenAction;

typedef int (*ScreenRegisterFn)(const ScreenAction* action);

struct ScreenRegisterEntry {
    ScreenRegisterFn fn; /* +0x00 */
    unsigned int id; /* +0x04 */
}; /* 0x08 */

/*
 * ScreenMgr -- Midway screen stack orchestrator (mwScreenEngine).
 *
 * Soft ceilings (codegen leftovers -- do not invent wrong algorithms):
 *   UpdateBranchPath ~81%  /  Reset ~87.6%  /  SplitPath ~84%
 *   FindParent ~84%  /  BroadcastEvent ~82%  /  ProcessRegisterActions ~86.2%
 *   FindScreen ~89%  /  InsertScreen ~91.7%
 * Prefer typed walks at retail offsets over Matching grind.
 *
 * =====================================================================
 * ScreenClient contract (retail LoadScreenSet path)
 * =====================================================================
 *
 * Game owns: Init / LoadScreen / Idle / Render / FireEvent / BroadcastEvent /
 * GetActiveScreen / Open/Close/Append/Remove / Dispose / LoadCompleted.
 *
 * ScreenClient via ScreenUtil: LoadScreenSet / UnloadScreenSet / CreateElement /
 * UnloadScreen / DoneLoadingSet / Malloc / Free / PreloadData / Pre/PostRender.
 *
 * Glue load_screen -> LoadScreen(path,1) -> insert_2d_obj(vtbl_screen_engine)
 * onto image.screen_obj_list -> render_2d_objs -> screen_engine_render -> Render.
 *
 * Retail pointer tables: m_stack[i] / m_branch[i] / m_confirm[i]. Disc SE* blobs
 * keep packed u32 slots at +4 after count (ILP32).
 *
 * Layout (retail GC):
 *   +0x00 vtbl
 *   +0x14 flags (1 after Reset)
 *   +0x18 branch depth (-1 reset)
 *   +0x1c ScreenSet* m_branch[16]
 *   +0x5c / +0xe4 / +0x164 path char buffers
 *   +0xdc root set, +0xe0 current set
 *   +0x1a4 active count (-1 empty), +0x1a8 Screen* m_stack[16]
 *   +0x22c embedded ScreenActionStack (0x1C)
 *   +0x248 m_confirm[4] -- also stage words (GetStage index 1..3 -> [0..2])
 *   +0x258/25c/260 register-action table + fire latch
 */

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

#endif
