#ifndef MWSCREENENGINE_GAMEVARIABLES_H
#define MWSCREENENGINE_GAMEVARIABLES_H

#include "mwScreenEngine/TextureCollection.h"

/*
 * GameVariables / GameVariableDispatcher -- menu option/collection ranges.
 *
 * BSS `game_variables` (Glue island) is one GameVariables (size 0x1C).
 * ScreenControl::RegisterGameVariables chains it onto the static dispatcher.
 *
 * Dispatcher Get/Set helpers walk m_head via IsValidOptionRange /
 * IsValidCollectionRange, then call through GameVariables vtbl (subclass
 * fills GetInt/SetInt/...). Base __vt__13GameVariables leaves GetInt/SetInt
 * NULL -- only range helpers + empty HandleAction/Event stubs live here.
 */

class ScreenMgr;
class ScreenAction;
class ScreenObject;

class GameVariables {
public:
    GameVariables();

    /* Non-virtual range helpers (retail free functions on this). */
    unsigned int IsValidInt(int a, int b, int c);
    unsigned int IsValidOptionRange(unsigned int id);
    unsigned int IsValidCollectionRange(unsigned int id);
    void SetOptionRange(unsigned int minId, unsigned int maxId);
    void SetCollectionRange(unsigned int minId, unsigned int maxId);

    /* Base vtbl stubs (retail returns / no-ops). */
    int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    void HandleEvent(ScreenObject* object, int event, int arg);
    int GetRowState(int a, int b);
    void SetRowState(int a, int b, int c);
    int GetColState(int a, int b);
    void SetColState(int a, int b, int c);
    int IsValidOption(int id);

    void* m_vtbl; /* +0x00 -- retail __vt__13GameVariables */
    int m_optMin; /* +0x04 */
    int m_optMax; /* +0x08 */
    int m_colMin; /* +0x0c */
    int m_colMax; /* +0x10 */
    int m_pad14; /* +0x14 -- ctor leaves unset */
    GameVariables* m_next; /* +0x18 -- dispatcher chain */
};

class GameVariableDispatcher {
public:
    GameVariableDispatcher();

    void Register(GameVariables* vars);

    int IsValidInt(unsigned int a, unsigned int b, unsigned int c, unsigned int id,
                   int value);
    int GetInt(unsigned int unused, unsigned int id);
    void SetInt(unsigned int unused, unsigned int id, int value);
    char* GetString(unsigned int unused, unsigned int id);
    void GetIntArray(unsigned int unused, unsigned int id, int* out, int count);
    void SetIntArray(unsigned int unused, unsigned int id, int* values, int count);
    void SetString(unsigned int unused, unsigned int id, char* str);
    int GetRowState(unsigned int unused, unsigned int id, int row);
    int GetColState(unsigned int unused, unsigned int id, int col);
    void SetRowState(unsigned int unused, unsigned int id, int row, int value);
    void SetColState(unsigned int unused, unsigned int id, int col, int value);
    int IsValidOption(unsigned int unused, unsigned int id);
    int GetStringCollection(unsigned int unused, unsigned int id, char*** out);
    int GetStringMatrixCollection(unsigned int unused, unsigned int id, char*** out,
                                  int& rows);
    void FreeStringCollection(unsigned int unused, unsigned int id, char** strings,
                              unsigned int count);
    int GetTextureCollection(unsigned int unused, int id, GMTextureInfo_t* out,
                             unsigned int& count);
    void FreeTextureCollection(unsigned int unused, int id, GMTextureInfo_t* info);

    int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    void HandleEvent(ScreenObject* object, int event, int arg);

    /* Thin wrappers: unused slot = (unsigned)-1. */
    int GetInt(int id);
    void SetString(int id, char* str);
    int GetTextureCollection(int id, GMTextureInfo_t* out, unsigned int& count);
    void FreeTextureCollection(int id, GMTextureInfo_t* info);

    GameVariables* m_head; /* +0x00 */
};

#endif
