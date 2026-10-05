/*
 * ScreenInstancer.o -- build and tear down Screen trees from SCREEN blobs.
 *
 * Pipeline: LoadScreenSet -> LoadSetData(SSET) -> ProcessScreenData(SNGC) ->
 * CreateScreen -> CreateObject / CreateElements -> CreateElement.
 * Packed pointer slots contain file-relative offsets until the patch pass.
 */

#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenSet.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenClient.h"
#include "mwScreenEngine/ScreenParams.h"

#include "runtime/cstring.h"

enum {
    kMagicSSET = 0x53534554, /* 'SSET' */
    kMagicDONE = 0x444F4E45, /* 'DONE' */
    kMagicSNGC = 0x534E4743, /* 'SNGC' */
    kMallocTag = 0x494E4954  /* 'INIT' */
};

#define SCREEN_IDLE_EVENT 0x405

static void ProcessControls(SEObject_t* seObj);

int ScreenInstancer::CreateScreen(Screen* screen, SEScreen_t* seScreen) {
    SEObject_t* rootSe;
    ScreenObject* root;
    ScreenMgr* mgr;

    /* Retail loads m_set->m_mgr before the seScreen null branch. */
    mgr = screen->m_set->m_mgr;
    if (seScreen == 0) {
        return 0;
    }

    rootSe = (SEObject_t*)seScreen->objects;
    root = CreateObject(mgr, screen, 0, rootSe);
    rootSe->liveObject = root;
    screen->m_data = seScreen;
    screen->InitMatrixStack();
    ProcessControls(rootSe);
    screen->m_loaded = 1;
    return rootSe->liveObject != 0;
}

void ScreenInstancer::DestroyObject(ScreenObject* object) {
    int i;
    int n;
    ScreenChildEntry* entry;
    ScreenChildList* list;
    ScreenObject* child;
    int tag;

    list = object->m_ext->children;
    if (list == 0) {
        return;
    }

    n = list->count;
    for (i = 0; i < n; i++) {
        entry = ScreenChildEntryAt(list, i);
        tag = entry->typeTag;
        switch (tag) {
        case kScreenTagOBJ:
        case kScreenTagGROP:
            DestroyObject(entry->object);
            break;
        }
        child = entry->object;
        child->Dispose();
        delete child;
        entry->object = 0;
    }
}

void ScreenInstancer::DestroyScreen(Screen* screen) {
    ScreenObject* root;

    root = screen->GetRoot();
    if (root != 0) {
        DestroyObject(root);
        root->m_ext->liveObject = 0;
        root->Dispose();
        delete root;
    }
}

void ScreenInstancer::CloseObject(ScreenObject* object) {
    int i;
    int n;
    ScreenChildEntry* entry;
    ScreenChildList* list;
    int tag;

    list = object->m_ext->children;
    if (list == 0) {
        return;
    }

    n = list->count;
    for (i = 0; i < n; i++) {
        entry = ScreenChildEntryAt(list, i);
        tag = entry->typeTag;
        switch (tag) {
        case kScreenTagOBJ:
        case kScreenTagGROP:
            CloseObject(entry->object);
            break;
        }
        entry->object->Close();
    }
}

void ScreenInstancer::CloseScreen(Screen* screen) {
    ScreenObject* root;

    root = screen->GetRoot();
    if (root != 0) {
        CloseObject(root);
    }
}

static void ProcessControls(SEObject_t* seObj) {
    ScreenObject* live;
    int i;
    int n;
    ScreenChildList* children;
    ScreenChildEntry* entry;
    int tag;
    if (seObj->classInfo != 0 && seObj->classInfo->params != 0) {
        live = seObj->liveObject;
        ((ScreenControl*)live)->ProcessParams(
            (ScreenParams*)seObj->classInfo->params);
        ((ScreenControl*)live)->Init();
    }

    children = seObj->children;

    i = 0;
    n = children->count;
    while (i < n) {
        entry = ScreenChildEntryAt(children, i);
        tag = entry->typeTag;
        switch (tag) {
        case kScreenTagOBJ:
        case kScreenTagGROP:
            ProcessControls((SEObject_t*)entry);
            break;
        }
        i += 1;
    }
}

ScreenObject* ScreenInstancer::CreateObject(ScreenMgr* mgr, Screen* screen,
                                            ScreenObject* parent, SEObject_t* seObj) {
    ScreenObject templateObj;
    ScreenObject* obj;
    SEObjectClassInfo* info;
    ScreenClient* client;
    ScreenObject* ancestor;

    info = seObj->classInfo;
    if (info == 0) {
        obj = (ScreenObject*)ScreenUtil::Malloc(sizeof(ScreenObject), kMallocTag,
                                                "SS-Objects");
        memcpy(obj, &templateObj, sizeof(ScreenObject));
    } else {
        client = ScreenUtil::GetScreenClient();
        obj = client->CreateInstance(
            mgr, info->typeId, (ScreenParams*)info->params);
        if (obj == 0) {
            obj = (ScreenObject*)ScreenUtil::Malloc(sizeof(ScreenObject), kMallocTag,
                                                    "SS-Objects");
            memcpy(obj, &templateObj, sizeof(ScreenObject));
            seObj->classInfo = 0;
        }
    }

    obj->m_ext = seObj;
    obj->m_screen = screen;

    ancestor = parent;
    while (ancestor != 0 && ancestor->m_ext->typeTag == kScreenTagGROP) {
        ancestor = ancestor->m_parent;
    }
    obj->SetParent(ancestor);

    if (seObj->transform != 0) {
        obj->CreateMatrixStack();
        obj->UpdateTransform();
    }

    if (obj->m_objTag == kScreenTagSCTL) {
        screen->SetHeadControl(obj);
        screen->SetHeadIdle(obj);
    } else if ((unsigned int)obj->HasEvent(SCREEN_IDLE_EVENT) != 0) {
        screen->SetHeadIdle(obj);
    }

    CreateElements(mgr, screen, obj, seObj->children);
    return obj;
}

int ScreenInstancer::CreateElements(ScreenMgr* mgr, Screen* screen, ScreenObject* parent,
                                    SEElements_t* elements) {
    int i;
    int n;
    int tag;
    ScreenChildEntry* entry;
    ScreenObject* created;

    i = 0;
    n = elements->count;
    while (i < n) {
        entry = ScreenChildEntryAt(elements, i);
        tag = entry->typeTag;
        switch (tag) {
        case kScreenTagOBJ:
            created = CreateObject(mgr, screen, parent, (SEObject_t*)entry);
            break;
        case kScreenTagGROP:
            created = CreateObject(mgr, screen, parent, (SEObject_t*)entry);
            break;
        default:
            created =
                (ScreenObject*)ScreenUtil::CreateElement(mgr, screen, parent, entry);
            break;
        }
        entry->object = created;
        if (created != 0 && (unsigned int)created->NeedIdleProcessing() != 0) {
            screen->SetHeadIdle(created);
        }
        i += 1;
    }
    return 1;
}

void PatchTextObject(SEBaseElement_t* baseElem, unsigned char* /*base*/,
                     SEStringTable_t* strings) {
    SETextElement_t* elem = (SETextElement_t*)baseElem;
    unsigned int idx;

    idx = elem->string0;
    if (idx < strings->count) {
        elem->string0 = (unsigned int)strings->strings[idx];
    } else {
        elem->string0 = 0;
    }

    idx = elem->string1;
    if (idx < strings->count) {
        elem->string1 = (unsigned int)strings->strings[idx];
    } else {
        elem->string1 = 0;
    }
}

void PatchPolyObject(SEBaseElement_t* baseElem, unsigned char* /*base*/,
                     SEStringTable_t* strings) {
    SEPolyElement_t* elem = (SEPolyElement_t*)baseElem;
    unsigned int idx;

    idx = elem->textureString;
    if (idx < strings->count) {
        elem->textureString = (unsigned int)strings->strings[idx];
    } else {
        elem->textureString = 0;
    }
}

void PatchAttribue(SEBaseAttribute_t* attr, unsigned char* base,
                   SEStringTable_t* strings) {
    int type;
    unsigned int idx;

    type = attr->type;
    switch (type) {
    case 4:
    case 7:
        idx = attr->value;
        if (idx < strings->count) {
            attr->value = (unsigned int)strings->strings[idx];
            return;
        }
        attr->value = 0;
        return;
    case 5:
    case 6:
        if (attr->value != 0) {
            attr->value += (unsigned int)base;
        }
        return;
    default:
        return;
    }
}

/* The list count is reloaded after each attribute is patched. */
/* TODO: [near miss] 92.0%; indexed member accesses differ; combined loop-mode control regresses siblings. */
void PatchAttribueList(SEAttributes_t* list, unsigned char* base,
                       SEStringTable_t* strings) {
    for (unsigned int i = 0; i < list->count; ++i) {
        SEBaseAttribute_t*& attribute = list->attributes[i];
        if (attribute != 0) {
            attribute = (SEBaseAttribute_t*)((unsigned char*)attribute + (unsigned int)base);
        }
        PatchAttribue(attribute, base, strings);
    }
}

/* Retail: no action null guard; reloc then reload attrs before Patch list. */
void PatchScreenAction(SEAction_t* action, unsigned char* base,
                       SEStringTable_t* strings) {
    unsigned int attributes = (unsigned int)action->attrs;

    if (attributes != 0) {
        action->attrs = (SEAttributes_t*)(attributes + (unsigned int)base);
    }
    if (action->attrs != 0) {
        PatchAttribueList(action->attrs, base, strings);
    }
}

/* Retail: no event null guard; inline actions at +0x0C stride 0x10. */
void PatchScreenEvent(SEEvent_t* event, unsigned char* base,
                      SEStringTable_t* strings) {
    unsigned int i;

    i = 0;
    while (i < event->actionCount) {
        PatchScreenAction(&event->actions[i], base, strings);
        i += 1;
    }
}

/* TODO: [near miss] 96.31%; eight event/child slot-addressing instructions remain. */
void PatchScreenObject(SEObject_t* obj, unsigned char* base,
                       SEStringTable_t* strings) {
    unsigned int i;
    unsigned int offset;
    ScreenEventList* events;
    ScreenChildList* children;
    ScreenChildEntry* entry;
    int tag;
    SEObjectClassInfo* classInfo;

    offset = (unsigned int)obj->events;
    if (offset != 0) {
        obj->events = (ScreenEventList*)(offset + (unsigned int)base);
    }
    offset = (unsigned int)obj->transform;
    if (offset != 0) {
        obj->transform = (SETransform*)(offset + (unsigned int)base);
    }
    offset = (unsigned int)obj->children;
    if (offset != 0) {
        obj->children = (ScreenChildList*)(offset + (unsigned int)base);
    }
    offset = (unsigned int)obj->classInfo;
    if (offset != 0) {
        obj->classInfo = (SEObjectClassInfo*)(offset + (unsigned int)base);
    }

    classInfo = SeClassInfoOf(obj);
    if (classInfo != 0) {
        offset = (unsigned int)classInfo->params;
        if (offset != 0) {
            classInfo->params =
                (SEAttributes_t*)(offset + (unsigned int)base);
        }
        classInfo = SeClassInfoOf(obj);
        if (classInfo->params != 0) {
            PatchAttribueList(classInfo->params, base, strings);
        }
    }

    events = SeEventsOf(obj);
    if (events != 0) {
        i = 0;
        while (i < (unsigned int)events->count) {
            SEEvent_t*& event = *SEEventPtrSlot(events, i);
            offset = (unsigned int)event;
            if (offset != 0) {
                event = (SEEvent_t*)(offset + (unsigned int)base);
            }
            PatchScreenEvent(event, base, strings);
            i += 1;
        }
    }

    children = SeChildrenOf(obj);
    if (children != 0) {
        i = 0;

        while (i < (unsigned int)children->count) {
            ScreenChildEntry*& child = *ScreenChildEntryPtrSlot(children, i);
            offset = (unsigned int)child;
            if (offset != 0) {
                child =
                    (ScreenChildEntry*)(offset + (unsigned int)base);
            }
            entry = child;
            if (entry != 0) {
                tag = entry->typeTag;
                switch (tag) {
                case kScreenTagPART:
                case kScreenTagCHAR:
                    break;
                case kScreenTagGROP:
                case kScreenTagOBJ:
                    PatchScreenObject((SEObject_t*)entry, base, strings);
                    break;
                case kScreenTagPOLY:
                    PatchPolyObject((SEBaseElement_t*)entry, base, strings);
                    break;
                case kScreenTagTEXT:
                    PatchTextObject((SEBaseElement_t*)entry, base, strings);
                    break;
                }
            }
            i += 1;
        }
    }
}

/* Retail reloc is bare fileOff+base (no SeFileReloc bl / null helper).
 * Slot loads use (base+off)->field via add+lwz, not lwzx(off+4). */
/* TODO: [near miss] 81.796875%; nested relocation flow agrees; typed table addressing and register allocation remain. */
void PatchAnimEffects(SEAnimEffects_t* effects, unsigned char* base) {
    int i;
    int j;
    int k;
    SEAnimEffect_t* effect;
    SEAnimEffect_t** effectSlot;
    SERefTable* tracks;
    SEAnimEffectItem_t* item;
    SERefTable* keys;
    unsigned int off;

    /* Retail: no effects==0 guard; count walk assumes valid block. */
    i = 0;
    while (i < effects->count) {
        effectSlot = SEAnimEffectPtrSlot(effects, i);
        off = (unsigned int)*effectSlot;
        if (off != 0) {
            *effectSlot = (SEAnimEffect_t*)(base + (unsigned int)*effectSlot);
        }
        effect = *effectSlot;
        if (effect != 0) {
            off = (unsigned int)effect->m_tracks;
            if (off != 0) {
                effect->m_tracks =
                    (SERefTable*)(base + (unsigned int)effect->m_tracks);
            }
            /* Retail reloads effect then walks tracks with no null check. */
            effect = *effectSlot;
            tracks = effect->m_tracks;
            j = 0;
            while (j < (int)tracks->count) {
                off = tracks->refs[j];
                if (off != 0) {
                    tracks->refs[j] = off + (unsigned int)base;
                }
                item = (SEAnimEffectItem_t*)tracks->refs[j];
                off = (unsigned int)item->m_keys;
                if (off != 0) {
                    item->m_keys =
                        (SERefTable*)(base + (unsigned int)item->m_keys);
                }
                keys = item->m_keys;
                k = 0;
                while (k < (int)keys->count) {
                    off = keys->refs[k];
                    if (off != 0) {
                        keys->refs[k] = off + (unsigned int)base;
                    }
                    k += 1;
                }
                j += 1;
            }
        }
        i += 1;
    }
}

/* TODO: [near miss] 97.60%; three reference-slot addressing instructions remain. */
void PatchAnims(SEAnimBlock_t* block, unsigned char* base) {
    unsigned int i;
    unsigned int j;
    ScreenAnimScene* scene;
    SERefTable* keyList;
    SEAnimSceneData_t* data;
    SEAnimTrack_t* track;
    unsigned int off;

    /* Retail: no block==0 guard; packed scene table uses its typed accessor. */
    i = 0;
    while (i < (unsigned int)block->count) {
        scene = ScreenAnimSceneAt(block, i);

        off = (unsigned int)scene->m_elements;
        if (off != 0) {
            scene->m_elements =
                (SEElements_t*)(off + (unsigned int)base);
        }
        off = (unsigned int)scene->m_data;
        if (off != 0) {
            scene->m_data =
                (SEAnimSceneData_t*)(off + (unsigned int)base);
        }

        /* Retail walks m_elements (SERefTable shape) with no null check after reloc. */
        keyList = (SERefTable*)scene->m_elements;
        j = 0;
        while (j < keyList->count) {
            off = keyList->refs[j];
            if (off != 0) {
                keyList->refs[j] = off + (unsigned int)base;
            }
            j += 1;
        }

        scene->m_flags = 0x20;

        /* The packed animation data is required after scene relocation. */
        data = scene->m_data;
        if ((int)(data->flags & 1) <= 0) {
            int trackIndex = 0;
            while (trackIndex < data->trackCount) {
                track = SEAnimTrackAt(data, trackIndex);
                off = (unsigned int)track->effects;
                if (off != 0) {
                    track->effects =
                        (SEAnimEffects_t*)(off + (unsigned int)base);
                }
                PatchAnimEffects(track->effects, base);
                trackIndex += 1;
            }
            scene->m_data->flags = 1;
            scene->CalculateMaxTime();
        }

        i += 1;
    }
}

void ProcessScreenData(Screen* /*screen*/, void* data, unsigned int /*size*/,
                       void* /*unused*/) {
    ScreenData* se;
    unsigned int base;
    unsigned int i;
    SEStringTable_t* strings;
    SeRef* stringSlot;
    SeRef strOff;

    if (data == 0) {
        return;
    }
    if (((ScreenData*)data)->magic != kMagicSNGC) {
        return;
    }

    base = (unsigned int)data;
    se = (ScreenData*)base;

    if (se->animScenes != 0) {
        se->animScenes =
            (SEAnimBlock_t*)((unsigned int)se->animScenes + base);
    }
    if (se->objects != 0) {
        se->objects =
            (ScreenObjectRoot*)((unsigned int)se->objects + base);
    }
    if (se->strings != 0) {
        se->strings =
            (SEStringTable_t*)((unsigned int)se->strings + base);
    }

    if (SeAnimScenesOf(se) != 0) {
        PatchAnims(SeAnimScenesOf(se), (unsigned char*)base);
    }

    if (SeStringsOf(se) != 0) {
        i = 0;
        while (i < SeStringsOf(se)->count) {
            strings = SeStringsOf(se);
            stringSlot = &strings->stringRefs[i];
            strOff = strings->stringRefs[i];
            if (strOff != 0) {
                *stringSlot = strOff + base;
            }
            i += 1;
        }
    }

    PatchScreenObject((SEObject_t*)SeObjectsOf(se), (unsigned char*)base,
                      SeStringsOf(se));
}

/* TODO: [near miss] 85.08871%; relocation-flag lowering and name-table scheduling remain. */
int ScreenInstancer::LoadSetData(ScreenSet* set, void* data, unsigned int /*size*/,
                                 void* /*unused*/) {
    SEScreenSet_t* blob;
    unsigned int base;
    Screen templateScreen;
    unsigned char needReloc;
    unsigned char relocFlag;
    int numScreens;
    int i;
    int screenByte;
    int ok;
    unsigned int magic;
    unsigned int nameOff;
    unsigned int screenOff;
    unsigned int* screenTable;
    unsigned int* nameRefs;
    Screen* screen;

    blob = (SEScreenSet_t*)data;
    base = (unsigned int)data;
    needReloc = 1;
    ok = 0;
    if (blob == 0) {
        return 0;
    }

    magic = blob->magic;
    if (magic == kMagicDONE) {
        needReloc = 0;
        magic = kMagicSSET;
    }

    if (magic == kMagicSSET) {
        numScreens = blob->numScreens;
        set->m_numScreens = numScreens;
        set->m_screens = (Screen*)ScreenUtil::Malloc((unsigned long)numScreens * kScreenBytes,
                                                     kMallocTag, "SS-Screens");
        i = 0;
        screenByte = 0;
        while (i < numScreens) {
            memcpy((char*)set->m_screens + screenByte, &templateScreen, kScreenBytes);
            i += 1;
            screenByte += kScreenBytes;
        }

        if (needReloc != 0 && blob->screenOffs != 0) {
            blob->screenOffs =
                (unsigned int*)((unsigned int)blob->screenOffs + base);
        }

        relocFlag = needReloc;
        nameRefs = SEScreenNameRefSlots(blob);
        screenByte = 0;
        for (i = 0; i < numScreens; i++) {
            if (relocFlag != 0) {
                nameOff = nameRefs[i];
                if (nameOff != 0) {
                    nameRefs[i] = nameOff + base;
                }
            }

            screen = (Screen*)((char*)set->m_screens + screenByte);
            screen->SetName((char*)nameRefs[i]);
            screen->m_set = set;

            screenTable = blob->screenOffs;
            if (screenTable != 0) {
                screenOff = screenTable[i];
                if (screenOff != 0) {
                    if (needReloc != 0) {
                        if (screenOff != 0) {
                            screenTable[i] = base + screenOff;
                        }
                        screen->m_data =
                            (SEScreen_t*)screenTable[i];
                        ProcessScreenData(
                            screen, (void*)screenTable[i], 0, 0);
                    }
                    CreateScreen(
                        screen,
                        (SEScreen_t*)screenTable[i]);
                }
            }
            screenByte += kScreenBytes;
        }

        set->m_inited = 1;
        set->m_setData = blob;
        set->DoneLoadingScreens();
        ok = 1;
    }

    blob->magic = kMagicDONE;
    return ok;
}
