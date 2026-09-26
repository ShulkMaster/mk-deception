#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenClient.h"

#define SCREEN_HEX_DIGIT_COUNT 4
#define SCREEN_HEX_LETTER_BASE 10

ScreenClient* ScreenUtil::m_pClient;

void ScreenUtil::ReportError(char* msg, char* file, int line) {
    if (m_pClient != 0) {
        m_pClient->ReportError(msg, file, line);
    }
}

void ScreenUtil::SetScreenClient(ScreenClient* client) {
    m_pClient = client;
}

ScreenClient* ScreenUtil::GetScreenClient() {
    return m_pClient;
}

void* ScreenUtil::Malloc(unsigned long size, int tag, char* name) {
    return m_pClient->Malloc(size, tag, name);
}

void ScreenUtil::Free(void* p) {
    m_pClient->Free(p);
}

ScreenMatrixStack* ScreenUtil::CreateMatrixStack() {
    return m_pClient->CreateMatrixStack();
}

void ScreenUtil::DestroyMatrixStack(ScreenMatrixStack* stack) {
    m_pClient->DestroyMatrixStack(stack);
}

void* ScreenUtil::CreateElement(ScreenMgr* mgr, Screen* screen, ScreenObject* obj,
                                void* data) {
    return m_pClient->CreateElement(mgr, screen, obj, data);
}

ScreenResourceLibrary* ScreenUtil::CreateResourceLibrary(ScreenResourceLibrary* parent) {
    return m_pClient->CreateResourceLibrary(parent);
}

void ScreenUtil::DestroyResourceLibrary(ScreenResourceLibrary* lib) {
    m_pClient->DestroyResourceLibrary(lib);
}

void ScreenUtil::PreRender() {
    m_pClient->PreRender();
}

void ScreenUtil::PostRender() {
    m_pClient->PostRender();
}

void ScreenUtil::SetCurrent(ScreenSet* set) {
    m_pClient->SetCurrent(set);
}

void ScreenUtil::Reset() {
    m_pClient->Reset();
}

int ScreenUtil::ReadHexInt(char* hex4) {
    unsigned int digits[SCREEN_HEX_DIGIT_COUNT];
    int i;

    digits[0] = hex4[0];
    digits[1] = hex4[1];
    digits[2] = hex4[2];
    digits[3] = hex4[3];

    for (i = 0; i < SCREEN_HEX_DIGIT_COUNT; i++) {
        if (digits[i] >= '0' && digits[i] <= '9') {
            digits[i] = digits[i] - '0';
        } else if (digits[i] >= 'A' && digits[i] <= 'F') {
            digits[i] -= 'A';
            digits[i] += SCREEN_HEX_LETTER_BASE;
        }
    }

    return (digits[0] << 12) | (digits[1] << 8) | (digits[2] << 4) | digits[3];
}

void ScreenUtil::UnloadScreen(Screen* screen) {
    m_pClient->UnloadScreen(screen);
}

void ScreenUtil::PreloadData(int id) {
    m_pClient->PreloadData(id);
}

int ScreenUtil::IsPreloadDataDone(int id) {
    return m_pClient->IsPreloadDataDone(id);
}

int ScreenUtil::DoneLoadingSet(ScreenSet* set) {
    return m_pClient->DoneLoadingSet(set);
}

int ScreenUtil::LoadScreenSet(ScreenSet* set) {
    return m_pClient->LoadScreenSet(set);
}

void ScreenUtil::UnloadScreenSet(int id) {
    m_pClient->UnloadScreenSet(id);
}

void ScreenUtil::SetRootTransformation(ScreenMatrixStack* stack) {
    m_pClient->SetRootTransformation(stack);
}

void ScreenUtil::HandleEvent(ScreenObject* obj, int a, int b) {
    m_pClient->HandleEvent(obj, a, b);
}

void ScreenUtil::HandleAction(ScreenMgr* mgr, const ScreenAction* action, int a) {
    m_pClient->HandleAction(mgr, action, a);
}

ScreenAction* ScreenUtil::CreateAction(int type) {
    return m_pClient->CreateAction(type);
}
