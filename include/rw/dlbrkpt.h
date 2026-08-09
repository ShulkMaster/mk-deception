#ifndef RW_DLBRKPT_H
#define RW_DLBRKPT_H

typedef void (*RwGxBreakPtCallback)(void);
typedef void (*RwGxDrawDoneUserCallback)(void* data);

void MWY_GCN_RW_ActivateGxBreakPtQueue(void);
void MWY_GCN_RW_SetGxBreakPtCallback(RwGxBreakPtCallback callback);
void MWY_GCN_RW_InsertGxDrawDoneCallback(
    RwGxDrawDoneUserCallback callback, void* data);
void MWY_GCN_RW_ActivateGxBreakPt(void* address);
void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void);
void MWY_GCN_RW_InsertRwGxBreakPt(void* address);
void MWY_GCN_RW_NoteRwGxBreakPt(void* address);

#endif
