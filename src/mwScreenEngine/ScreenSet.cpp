#include "mwScreenEngine/ScreenSet.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenUtil.h"

#define SCREEN_SET_NAME_CAPACITY 0x50U
#define SCREEN_SET_MAX_CHILDREN 16U
#define SCREEN_SET_ALLOC_TAG 0x494E4954

extern "C" {
char* strcpy(char* dst, const char* src);
unsigned long strlen(const char* s);
int strcmp(const char* a, const char* b);
int stricmp(const char* a, const char* b);
void* memcpy(void* dst, const void* src, unsigned long n);
}

ScreenSet::ScreenSet() {
    /* Soft ceiling: ~83% - retail string pool in .rodata; m_mgr left uninit. */
    m_numChildren = 0;
    m_parent = 0;
    strcpy(m_name, "");
    m_resourceLib = 0;
    m_inited = 0;
    m_numScreens = 0;
    m_screens = 0;
    m_setData = 0;
    m_unloadId = 0;
}

ScreenSet::~ScreenSet() {}

int ScreenSet::Init() {
    if (m_resourceLib == 0) {
        ScreenResourceLibrary* parentLib;
        if (m_parent != 0) {
            parentLib = m_parent->m_resourceLib;
        } else {
            parentLib = 0;
        }
        m_resourceLib = ScreenUtil::CreateResourceLibrary(parentLib);
    }
    if (m_resourceLib == 0) {
        m_resourceLib = 0;
    }
    return ScreenUtil::LoadScreenSet(this);
}

void ScreenSet::DoneLoadingScreens() {
    m_mgr->LoadCompleted(this);
    ScreenUtil::DoneLoadingSet(this);
}

void ScreenSet::Dispose() {
    if (m_screens != 0) {
        int n;
        /* Retail countdown: Dispose ScreenAt(base, n-1) after m_numScreens--. */
        while ((n = m_numScreens, m_numScreens = n - 1, n != 0)) {
            ScreenAt(m_screens, m_numScreens)->Dispose();
        }
        ScreenUtil::Free(m_screens);
        m_screens = 0;
        m_numScreens = 0;
    }
    if (m_setData != 0) {
        m_setData = 0;
    }
    if (m_resourceLib != 0) {
        ScreenUtil::DestroyResourceLibrary(m_resourceLib);
        m_resourceLib = 0;
    }
    m_inited = 0;
}

int ScreenSet::IsInited() const {
    unsigned int bit = m_inited & 1;
    return ((unsigned int)(-bit) & ~bit) >> 31;
}

char* ScreenSet::GetName() {
    return m_name;
}

void ScreenSet::SetName(char* name) {
    if (name != 0) {
        if (strlen(name) < SCREEN_SET_NAME_CAPACITY) {
            strcpy(m_name, name);
        }
    }
}

int ScreenSet::GetNumChildren() const {
    return m_numChildren;
}

ScreenSet* ScreenSet::GetChild(int index) {
    ScreenSet* child = 0;
    if (index >= 0 && index < m_numChildren) {
        child = m_children[index];
    }
    return child;
}

ScreenSet* ScreenSet::GetChild(char* name) {
    int index = GetChildIndex(name);

    return GetChild(index);
}

int ScreenSet::GetChildIndex(char* name) {
    int found = -1;
    int i;

    for (i = 0; i < m_numChildren; i++) {
        if (stricmp(m_children[i]->GetName(), name) == 0) {
            found = i;
            break;
        }
    }
    return found;
}

void ScreenSet::RemoveChild(ScreenSet* child) {
    /* Soft ceiling: ~63% - countdown/memcpy addressing near-miss. */
    int n = m_numChildren;
    int i = n;

    while (i != 0) {
        i -= 1;
        if (m_children[i] == child) {
            /* retail copies (n - i) pointers (one past logical end) */
            memcpy(&m_children[i], &m_children[i + 1],
                   (unsigned long)(n - i) * sizeof(ScreenSet*));
            m_numChildren -= 1;
            break;
        }
    }
}

void ScreenSet::AddChild(ScreenSet* child) {
    /* Soft ceiling: ~79% - stmw vs split stw prologue leftover. */
    if ((unsigned int)m_numChildren < SCREEN_SET_MAX_CHILDREN) {
        child->SetParent(this);
        int idx = m_numChildren;
        m_numChildren = idx + 1;
        m_children[idx] = child;
    }
}

ScreenSet* ScreenSet::GetParent() {
    return m_parent;
}

void ScreenSet::SetParent(ScreenSet* parent) {
    m_parent = parent;
}

Screen* ScreenSet::GetScreen(char* name) {
    Screen* screen = 0;
    int index;

    index = GetScreenIndex(name);
    if (index >= 0 && index < m_numScreens) {
        screen = &m_screens[index];
    }
    return screen;
}

int ScreenSet::GetScreenIndex(char* name) {
    /* Soft ceiling: GetScreenIndex ~99.3% -- i/offset addi order; stop. */
    int found = -1;
    int i;

    for (i = 0; i < m_numScreens; i++) {
        if (strcmp(m_screens[i].GetName(), name) == 0) {
            found = i;
            break;
        }
    }
    return found;
}

void ScreenSet::BroadcastEvent(ScreenMgr* mgr, int event, int arg) {
    /* Q16: decl n before i -> n@r30, i@r29 (was flipped). */
    int n;
    int i;

    n = m_numScreens;
    if (n == 0) {
        return;
    }
    for (i = 0; i < n; i++) {
        m_screens[i].BroadcastEvent(mgr, event, arg);
    }
}

void* ScreenSet::operator new(unsigned long size) {
    /* Soft ceiling: ~79% - retail @stringBase0+1; our strings land in sdata. */
    return ScreenUtil::Malloc(size, SCREEN_SET_ALLOC_TAG, (char*)"SS-Set");
}

void ScreenSet::operator delete(void* p) {
    ScreenUtil::Free(p);
}
