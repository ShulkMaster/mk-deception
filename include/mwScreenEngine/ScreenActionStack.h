#ifndef MWSCREENENGINE_SCREENACTIONSTACK_H
#define MWSCREENENGINE_SCREENACTIONSTACK_H

/*
 * ScreenActionStack -- embedded in ScreenMgr at +0x22c (size 0x1C).
 *
 * Retail: Must-run Midway (link this TU):
 *   ctor / Dispose
 *   PushAction / Process / RemoveActions
 *   IsActionBlockingEvents
 *   CreateAction -- ScreenUtil::CreateAction first, then typed factory for
 *     focus/confirm/visible/open/insert/question/else. Anim FX ids soft-ceiling
 *     to base ScreenAction.
 * Soft ceiling: CreateAction ~80% -- new/ctor vs inlined vtbl poke.
 * Soft ceiling: ClearActions ~92% -- delete schedule; PushAction matched.
 * Soft ceiling: Process ~90% -- NV reg coloring; EndLocal ~87% branch epilogue;
 *   Dispose ~86% mr. walker.
 *
 * Layout (retail, seven words):
 *   +0x00 m_mgr          ScreenMgr* (latched during Process)
 *   +0x04 m_processing   reentrancy guard (0/1)
 *   +0x08 m_head         main doubly-linked list head
 *   +0x0C m_tail         main list tail
 *   +0x10 m_localHead    StartLocal staging list head
 *   +0x14 m_localTail    StartLocal staging list tail
 *   +0x18 m_localMode    0 = Push to main; 1 = Push to local
 */

class ScreenAction;
class ScreenMgr;
class ScreenSet;

class ScreenActionStack {
public:
    ScreenActionStack();
    void Dispose();

    int IsActionBlockingEvents();
    void StartLocal();
    int EndLocal();
    void PushAction(ScreenAction* action);
    void Process(ScreenMgr* mgr, int dt);
    void RemoveActions(ScreenSet* set);
    static ScreenAction* CreateAction(unsigned int type);

    /* Used by Screen*ScreenAction::Update (wait until this is head). */
    ScreenAction* GetHead() const { return m_head; }

private:
    void ClearActions(ScreenSet* set, ScreenAction** pTail, ScreenAction** pHead);

    ScreenMgr* m_mgr; /* +0x00 */
    unsigned int m_processing; /* +0x04 */
    ScreenAction* m_head; /* +0x08 */
    ScreenAction* m_tail; /* +0x0C */
    ScreenAction* m_localHead; /* +0x10 */
    ScreenAction* m_localTail; /* +0x14 */
    unsigned int m_localMode; /* +0x18 */
};

#endif
