/*
 * ScreenInstancer.o -- build / tear down Screen trees from SCREEN blobs.
 *
 * NonMatching readable lift. ASM remains linked on GC until Matching.
 *
 * Pipeline: LoadScreenSet -> LoadSetData(SSET) -> ProcessScreenData(SNGC) ->
 * CreateScreen -> CreateObject / CreateElements -> CreateElement.
 *
 * SeRef risk: CreateElements / PatchScreenObject still the likeliest source of
 * bad SeRef links if tag walks or child reloc drift -- prefer typed SE* walks
 * over inventing host-only tables. Soft reclaim OK; keep sha1 green.
 *
 * Soft ceilings (trust report.json):
 *   CreateElements ~74% -- OBJ lis/cmpw vs subis; stop.
 *   ProcessScreenData ~93% -- schedule; stop.
 *   PatchScreenObject ~75% -- tag-tree/reg color; stop.
 *   PatchText/Poly ~89% -- same size, branch/reg leftover; stop.
 *   PatchAttribue/List/Event 100%. PatchAnimEffects ~88.4% / PatchAnims ~95.1%;
 *   typed SERefTable relocation slots preserve the retail reloc order.
 *   CreateScreen 100%. PatchScreenAction 100%. DestroyScreen **100%**.
 */

#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenSet.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenClient.h"
#include "mwScreenEngine/ScreenParams.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned long n);
}

enum {
    kMagicSSET = 0x53534554, /* 'SSET' */
    kMagicDONE = 0x444F4E45, /* 'DONE' */
    kMagicSNGC = 0x534E4743, /* 'SNGC' */
    kMallocTag = 0x494E4954  /* 'INIT' */
};

#define SCREEN_IDLE_EVENT 0x405

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

/* Retail: no list null guard; bare add reloc; count reloaded each iteration. */
void PatchAttribueList(SEAttributes_t* list, unsigned char* base,
                       SEStringTable_t* strings) {
    unsigned int i;
    SEBaseAttribute_t** slot;

    i = 0;
    while (i < list->count) {
        slot = SEAttributePtrSlot(list, i);
        if (*slot != 0) {
            *slot = (SEBaseAttribute_t*)SeFileReloc(base, *slot);
        }
        PatchAttribue(*slot, base, strings);
        i += 1;
    }
}

/*
 * Soft ceiling was wrong earlier -- these are global in retail; keep tight
 * slwi/add form (no SEStringSlot helper) for Matching progress.
 */
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
        PatchScreenAction(SEActionAt(event, i), base, strings);
        i += 1;
    }
}

void PatchScreenObject(SEObject_t* obj, unsigned char* base,
                       SEStringTable_t* strings) {
    unsigned int i;
    ScreenEventList* events;
    ScreenChildList* children;
    ScreenChildEntry* entry;
    int tag;
    SEObjectClassInfo* classInfo;
    SEEvent_t** eventSlot;
    ScreenChildEntry** childSlot;
    int partTag;
    int gropTag;
    int objTag;
    int textTag;
    int polyTag;
    int charTag;

    if (obj->events != 0) {
        obj->events = (ScreenEventList*)SeFileReloc(base, obj->events);
    }
    if (obj->transform != 0) {
        obj->transform = (SETransform*)SeFileReloc(base, obj->transform);
    }
    if (obj->children != 0) {
        obj->children = (ScreenChildList*)SeFileReloc(base, obj->children);
    }
    if (obj->classInfo != 0) {
        obj->classInfo = (SEObjectClassInfo*)SeFileReloc(base, obj->classInfo);
    }

    classInfo = SeClassInfoOf(obj);
    if (classInfo != 0) {
        if (classInfo->params != 0) {
            classInfo->params =
                (SEAttributes_t*)SeFileReloc(base, classInfo->params);
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
            eventSlot = SEEventPtrSlot(events, i);
            if (*eventSlot != 0) {
                *eventSlot = (SEEvent_t*)SeFileReloc(base, *eventSlot);
            }
            PatchScreenEvent(*eventSlot, base, strings);
            i += 1;
        }
    }

    children = SeChildrenOf(obj);
    if (children != 0) {
        i = 0;
        partTag = (int)kScreenTagPART;
        gropTag = (int)kScreenTagGROP;
        objTag = (int)kScreenTagOBJ;
        textTag = (int)kScreenTagTEXT;
        polyTag = (int)kScreenTagPOLY;
        charTag = (int)kScreenTagCHAR;
        while (i < (unsigned int)children->count) {
            childSlot = ScreenChildEntryPtrSlot(children, i);
            if (*childSlot != 0) {
                *childSlot =
                    (ScreenChildEntry*)SeFileReloc(base, *childSlot);
            }
            entry = *childSlot;
            if (entry != 0) {
                tag = (int)entry->typeTag;
                /* Retail binary tree on PART / GROP / OBJ / TEXT / POLY / CHAR. */
                if (tag != partTag) {
                    if (tag < partTag) {
                        if (tag == gropTag) {
                            PatchScreenObject((SEObject_t*)entry, base, strings);
                        } else if (tag >= gropTag) {
                            if (tag == objTag) {
                                PatchScreenObject((SEObject_t*)entry, base, strings);
                            }
                        } else if (tag == charTag) {
                            /* skip */
                        }
                    } else if (tag == textTag) {
                        PatchTextObject((SEBaseElement_t*)entry, base, strings);
                    } else if (tag < textTag) {
                        if (tag == polyTag) {
                            PatchPolyObject((SEBaseElement_t*)entry, base, strings);
                        }
                    }
                }
            }
            i += 1;
        }
    }
}

/* Retail reloc is bare fileOff+base (no SeFileReloc bl / null helper).
 * Slot loads use (base+off)->field via add+lwz, not lwzx(off+4). */
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
            *effectSlot = (SEAnimEffect_t*)SeFileReloc(base, *effectSlot);
        }
        effect = *effectSlot;
        if (effect != 0) {
            off = (unsigned int)effect->m_tracks;
            if (off != 0) {
                effect->m_tracks =
                    (SERefTable*)SeFileReloc(base, effect->m_tracks);
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
                        (SERefTable*)SeFileReloc(base, item->m_keys);
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
                (SEElements_t*)SeFileReloc(base, scene->m_elements);
        }
        off = (unsigned int)scene->m_data;
        if (off != 0) {
            scene->m_data =
                (SEAnimSceneData_t*)SeFileReloc(base, scene->m_data);
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

        /* Retail: no data==0 guard. Soft ceiling: clrlwi./bgt vs andi./bne on bit0. */
        data = scene->m_data;
        if ((data->flags & 1) == 0) {
            j = 0;
            while ((int)j < data->trackCount) {
                track = SEAnimTrackAt(data, j);
                off = (unsigned int)track->effects;
                if (off != 0) {
                    track->effects =
                        (SEAnimEffects_t*)SeFileReloc(base, track->effects);
                }
                PatchAnimEffects(track->effects, base);
                j += 1;
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
    unsigned int* stringSlot;
    unsigned int strOff;

    if (data == 0) {
        return;
    }
    se = (ScreenData*)data;
    /* Retail: subis/cmplwi on 'SNGC' high/low halves. */
    if (se->magic != kMagicSNGC) {
        return;
    }

    base = (unsigned int)data;

    /* Reloc order matches retail: animScenes@+0x10, objects@+0x0C, strings@+0x14. */
    if (se->animScenes != 0) {
        se->animScenes =
            (SEAnimBlock_t*)SeFileReloc((unsigned char*)data, se->animScenes);
    }
    if (se->objects != 0) {
        se->objects =
            (ScreenObjectRoot*)SeFileReloc((unsigned char*)data, se->objects);
    }
    if (se->strings != 0) {
        se->strings =
            (SEStringTable_t*)SeFileReloc((unsigned char*)data, se->strings);
    }

    if (SeAnimScenesOf(se) != 0) {
        PatchAnims(SeAnimScenesOf(se), (unsigned char*)base);
    }

    /* Soft ceiling: ProcessScreenData -- string-loop reloads strings each iter. */
    if (SeStringsOf(se) != 0) {
        i = 0;
        while (i < SeStringsOf(se)->count) {
            strings = SeStringsOf(se);
            stringSlot = SEStringSlot(strings, i);
            strOff = *stringSlot;
            if (strOff != 0) {
                *stringSlot = strOff + base;
            }
            i += 1;
        }
    }

    PatchScreenObject((SEObject_t*)SeObjectsOf(se), (unsigned char*)base,
                      SeStringsOf(se));
}

static void ProcessControls(SEObject_t* seObj) {
    ScreenObject* live;
    ScreenChildList* children;
    ScreenChildEntry* entry;
    int i;
    int n;
    int tag;
    int objTag;
    int gropTag;
    /* Soft ceiling: ProcessControls -- OBJ/GROP lis/cmpw vs subis fold; stop. */
    if (seObj->classInfo != 0 && seObj->classInfo->params != 0) {
        live = seObj->liveObject;
        ((ScreenControl*)live)->ProcessParams(
            (ScreenParams*)seObj->classInfo->params);
        ((ScreenControl*)live)->Init();
    }

    children = seObj->children;
    objTag = (int)kScreenTagOBJ;
    gropTag = (int)kScreenTagGROP;
    i = 0;
    n = children->count;
    while (i < n) {
        entry = ScreenChildEntryAt(children, i);
        tag = (int)entry->typeTag;
        if (tag == objTag || (tag < objTag && tag == gropTag)) {
            ProcessControls((SEObject_t*)entry);
        }
        i += 1;
    }
}

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
    return (unsigned char)(rootSe->liveObject != 0);
}

/* Soft ceiling: DestroyObject / CloseObject / ProcessControls --
 * retail-sized loops + vtbl Dispose(+0x10)/dtor(+0x08)/Close(+0x38)/
 * Update(+0x44)/ProcessParams(+0x40)/Init(+0x0C); MWCC C++ still oversized vs retail. Stop.
 * DestroyScreen Matching 100% (Dispose + delete after clear liveObject). */
void ScreenInstancer::DestroyObject(ScreenObject* object) {
    ScreenChildList* list;
    ScreenChildEntry* entry;
    ScreenObject* child;
    int i;
    int n;
    int tag;
    int objTag;

    list = object->m_ext->children;
    if (list == 0) {
        return;
    }

    /* Retail: lis OBJ first; GROP only in bge-fail branch. */
    objTag = (int)kScreenTagOBJ;
    n = list->count;
    for (i = 0; i < n; i++) {
        entry = ScreenChildEntryAt(list, i);
        tag = (int)entry->typeTag;
        if (tag == objTag || (tag < objTag && tag == (int)kScreenTagGROP)) {
            DestroyObject(entry->object);
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
    ScreenChildList* list;
    ScreenChildEntry* entry;
    int i;
    int n;
    int tag;
    int objTag;
    int gropTag;

    /* Soft ceiling ~79%: OBJ/GROP lis/cmpw vs subis; typed entry walk. */
    list = object->m_ext->children;
    if (list == 0) {
        return;
    }

    gropTag = (int)kScreenTagGROP;
    objTag = (int)kScreenTagOBJ;
    n = list->count;
    for (i = 0; i < n; i++) {
        entry = ScreenChildEntryAt(list, i);
        tag = (int)entry->typeTag;
        if (tag == objTag || (tag < objTag && tag == gropTag)) {
            CloseObject(entry->object);
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

ScreenObject* ScreenInstancer::CreateObject(ScreenMgr* mgr, Screen* screen,
                                            ScreenObject* parent, SEObject_t* seObj) {
    ScreenObject templateObj;
    ScreenObject* obj;
    SEObjectClassInfo* info;
    ScreenClient* client;

    info = seObj->classInfo;
    if (info == 0) {
        obj = (ScreenObject*)ScreenUtil::Malloc(sizeof(ScreenObject), kMallocTag,
                                                (char*)"SS-Objects");
        memcpy(obj, &templateObj, sizeof(ScreenObject));
    } else {
        client = ScreenUtil::GetScreenClient();
        obj = client->CreateInstance(
            mgr, info->typeId, (ScreenParams*)info->params);
        if (obj == 0) {
            obj = (ScreenObject*)ScreenUtil::Malloc(sizeof(ScreenObject), kMallocTag,
                                                    (char*)"SS-Objects");
            memcpy(obj, &templateObj, sizeof(ScreenObject));
            seObj->classInfo = 0;
        }
    }

    obj->m_ext = seObj;
    obj->m_screen = screen;

    while (parent != 0 && parent->m_ext->typeTag == kScreenTagGROP) {
        parent = parent->m_parent;
    }
    obj->SetParent(parent);

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

    CreateElements(mgr, screen, obj, (SEElements_t*)seObj->children);
    return obj;
}

/*
 * Soft ceiling: CreateElements -- OBJ/GROP lis+cmpw vs subis fold; stop.
 * Retail: tag==OBJ || (tag<OBJ && tag==GROP) -> CreateObject else CreateElement.
 */
int ScreenInstancer::CreateElements(ScreenMgr* mgr, Screen* screen, ScreenObject* parent,
                                    SEElements_t* elements) {
    int objTag;
    int i;
    int n;
    int tag;
    ScreenChildEntry* entry;
    ScreenObject* created;

    /* Soft ceiling ~74%: OBJ lis/cmpw vs subis; typed entry walk. */
    objTag = (int)kScreenTagOBJ;
    i = 0;
    n = elements->count;
    while (i < n) {
        entry = ScreenChildEntryAt(elements, i);
        tag = (int)entry->typeTag;
        if (tag == objTag) {
            created = CreateObject(mgr, screen, parent, (SEObject_t*)entry);
        } else if (tag < objTag && tag == (int)kScreenTagGROP) {
            created = CreateObject(mgr, screen, parent, (SEObject_t*)entry);
        } else {
            created =
                (ScreenObject*)ScreenUtil::CreateElement(mgr, screen, parent, entry);
        }
        entry->object = created;
        if (created != 0 && (unsigned int)created->NeedIdleProcessing() != 0) {
            screen->SetHeadIdle(created);
        }
        i += 1;
    }
    return 1;
}

/* Soft ceiling: LoadSetData ~87.4% -- needReloc clrlwi / name-table schedule; stop. */
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
                                                     kMallocTag, (char*)"SS-Screens");
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
