/* BUILD: -inline off: GetChild(char*) must bl GetChild(int); GetScreen stmw. */

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

/* TODO: [near miss] 82.85%; retail's "" literal comes from the .rodata string pool (TU data layout). */
ScreenSet::ScreenSet() {
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
        while (m_numScreens-- != 0) {
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
    return (m_inited & 1) > 0;
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

/* TODO: [near miss] 87.20%; countdown loop and memcpy address formation differ. */
void ScreenSet::RemoveChild(ScreenSet* child) {
    int n = m_numChildren;
    int i = n;

    while (i != 0) {
        i -= 1;
        if (m_children[i] == child) {
            memcpy(&m_children[i], &m_children[i + 1],
                   (unsigned long)(n - i) * sizeof(ScreenSet*));
            m_numChildren -= 1;
            break;
        }
    }
}

void ScreenSet::AddChild(ScreenSet* child) {
    if ((unsigned int)m_numChildren < SCREEN_SET_MAX_CHILDREN) {
        child->SetParent(this);
        m_children[m_numChildren++] = child;
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

/* TODO: [near miss] 79.23%; retail "SS-Set" is @stringBase0+1 in .rodata, ours lands in .sdata (TU data layout). */
void* ScreenSet::operator new(unsigned long size) {
    return ScreenUtil::Malloc(size, SCREEN_SET_ALLOC_TAG, (char*)"SS-Set");
}

void ScreenSet::operator delete(void* p) {
    ScreenUtil::Free(p);
}
