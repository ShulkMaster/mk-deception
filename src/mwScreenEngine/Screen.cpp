#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenMatrixStack.h"

extern "C" {
char* strcpy(char* dst, const char* src);
}

/* Retail @120 -- flags/matrix + colorScale(1) + colorTranslation(0). */
static const ScreenRenderInfo s_renderInfoInit = {
    0,
    0,
    {1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
};

#define SCREEN_IDLE_EVENT 0x405

Screen::Screen() {
    /* Retail store order after strcpy(m_name, ""). */
    strcpy(m_name, "");
    m_data = 0;
    m_state = -1;
    m_visible = 1;
    m_loaded = 0;
    field_0x4C = 1;
    m_opened = 0;
    m_matrixStack = 0;
    m_headControl = 0;
    m_headIdle = 0;
    /* m_set left uninitialized in retail ctor. */
}

Screen::~Screen() {
    /* Retail: optional operator delete only; Dispose is separate. */
}

void Screen::Dispose() {
    if (m_data != 0) {
        ScreenUtil::UnloadScreen(this);
    }
    if (m_matrixStack != 0) {
        m_matrixStack->Dispose();
        ScreenUtil::DestroyMatrixStack(m_matrixStack);
        m_matrixStack = 0;
    }
}

ScreenAnimScene* Screen::GetAnimScene(int index) {
    ScreenData* data;
    ScreenAnimSceneList* scenes;

    /* Retail: (m_data != 0) && (index < count) -- no animScenes null check. */
    data = m_data;
    if (data != 0) {
        scenes = data->animScenes;
        if (index < scenes->count) {
            return ScreenAnimSceneAt(scenes, index);
        }
    }
    return 0;
}

void Screen::ShutoffAnimScenes() {
    ScreenData* data;
    ScreenAnimSceneList* scenes;
    int count;
    int offset;

    data = m_data;
    if (data == 0) {
        return;
    }
    scenes = data->animScenes;
    if (scenes == 0) {
        return;
    }
    /*
     * Soft ceiling: ShutoffAnimScenes 82.36% -- retail rotates the reverse loop.
     * Putting i before scenes gives the retail register order but crashes MWCC 2.7.
     */
    count = scenes->count;
    offset = count * 0x18;
    while (count != 0) {
        count -= 1;
        offset -= 0x18;
        ScreenAnimSceneAtOffset(scenes, offset)->Reset();
    }
}

void Screen::BroadcastEvent(ScreenMgr* mgr, int event, int arg) {
    ScreenObject* root;

    root = GetRoot();
    if (root != 0) {
        root->BroadcastEvent(mgr, event, arg);
    }
}

void Screen::UpdateSceneAnimation(int dt) {
    ScreenData* data;
    int i;
    int count;
    ScreenAnimSceneList* scenes;

    data = m_data;
    if (data == 0) {
        return;
    }
    scenes = data->animScenes;
    if (scenes == 0) {
        return;
    }
    count = scenes->count;
    for (i = 0; i < count; i++) {
        ScreenAnimSceneAt(scenes, i)->Process(dt);
    }
}

ScreenObject* Screen::GetRoot() {
    ScreenObject* root;
    ScreenData* data;
    ScreenObjectRoot* objects;

    root = 0;
    data = m_data;
    if (data != 0) {
        objects = data->objects;
        if (objects != 0) {
            root = objects->root;
        }
    }
    return root;
}

void Screen::FireEvent(ScreenMgr* mgr, int event, int arg, unsigned int flag) {
    ScreenObject* root = GetRoot();

    if (root != 0) {
        root->FireEvent(mgr, event, arg, flag);
    }
}

void Screen::InitMatrixStack() {
    ScreenObject* root;

    root = GetRoot();
    m_matrixStack = ScreenUtil::CreateMatrixStack();
    m_matrixStack->Init();
    ScreenUtil::SetRootTransformation(m_matrixStack);
    if (root != 0) {
        root->SetMatrixStack(m_matrixStack);
    }
}

void Screen::RenderAll() {
    ScreenData* data;
    ScreenObjectRoot* objects;
    /* Aggregate init -- avoid C++ __as__ (retail uses mtctr dword-pair of @120). */
    ScreenRenderInfo info = s_renderInfoInit;

    info.matrixStack = m_matrixStack;

    data = m_data;
    if (data == 0) {
        return;
    }
    objects = data->objects;
    if (objects == 0) {
        return;
    }
    if ((unsigned int)m_visible == 0) {
        return;
    }
    objects->root->Render(&info);
}

char* Screen::GetName() {
    return m_name;
}

void Screen::SetName(char* name) {
    strcpy(m_name, name);
}

void Screen::SetHeadIdle(ScreenNode* node) {
    node->next = m_headIdle;
    m_headIdle = node;
}

void Screen::ProcessIdleEvent(ScreenMgr* mgr) {
    ScreenNode* node;

    node = m_headIdle;
    while (node != 0) {
        if (node->typeTag == kScreenTagOBJ) {
            ScreenObject* obj = (ScreenObject*)node;
            obj->HandleEvent(mgr, SCREEN_IDLE_EVENT, 0);
            obj->ProcessEvent(mgr, SCREEN_IDLE_EVENT, 0);
        } else {
            node->ProcessEngineEvent(mgr, SCREEN_IDLE_EVENT);
        }
        node = node->next;
    }
}

void Screen::SetHeadControl(ScreenNode* node) {
    node->next = m_headControl;
    m_headControl = node;
}
