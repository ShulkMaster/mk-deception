#ifndef MWSCREENENGINE_GAMEVARIABLES_H
#define MWSCREENENGINE_GAMEVARIABLES_H

#include "mwScreenEngine/TextureCollection.h"

/*
 * GameVariables / GameVariableDispatcher -- menu option/collection ranges.
 *
 * GameVariables is the abstract base of the menu-variable providers: the
 * subclass supplies Get/Set for ints, floats, strings, arrays and collections;
 * the base supplies the option/collection range bookkeeping plus empty
 * HandleAction/HandleEvent/row/col stubs. Declaration order below IS the
 * retail vtable slot order (__vt__13GameVariables, .data 0x68 = 26 words:
 * 2 header words then slots 2..25); pure slots are the 0x00000000 entries.
 *
 * BSS `game_variables` (Glue island) is one such subclass (size 0x1C).
 * ScreenControl::RegisterGameVariables chains it onto the static dispatcher,
 * whose Get/Set helpers walk m_next via the non-virtual IsValidOptionRange /
 * IsValidCollectionRange and then make an ordinary virtual call on the node.
 */

class ScreenMgr;
class ScreenAction;
class ScreenObject;

class GameVariables {
public:
    GameVariables();

    /* Virtuals in vtable slot order -- do not reorder. */
    virtual void Init() = 0; /* slot 2 */
    virtual void Dispose() = 0; /* slot 3 */
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action); /* slot 4 */
    virtual void HandleEvent(ScreenObject* object, int event, int arg); /* slot 5 */
    virtual int GetInt(int id) = 0; /* slot 6 */
    virtual unsigned int IsValidInt(int a, int b, int c); /* slot 7 */
    virtual void SetInt(int id, int value) = 0; /* slot 8 */
    virtual float GetFloat(int id) = 0; /* slot 9 */
    virtual void SetFloat(int id, float value) = 0; /* slot 10 */
    virtual char* GetString(int id) = 0; /* slot 11 */
    virtual void SetString(int id, char* str) = 0; /* slot 12 */
    virtual void GetIntArray(int id, int* out, int count) = 0; /* slot 13 */
    virtual void SetIntArray(int id, int* values, int count) = 0; /* slot 14 */
    virtual int IsValidOption(int id); /* slot 15 */
    virtual int GetRowState(int id, int row); /* slot 16 */
    virtual void SetRowState(int id, int row, int value); /* slot 17 */
    virtual int GetColState(int id, int col); /* slot 18 */
    virtual void SetColState(int id, int col, int value); /* slot 19 */
    virtual int GetStringCollection(int id, char*** out) = 0; /* slot 20 */
    virtual int GetStringMatrixCollection(int id, char*** out, int& rows) = 0; /* slot 21 */
    virtual void FreeStringCollection(int id, char** strings,
                                      unsigned int count) = 0; /* slot 22 */
    virtual int GetNumStrings(int id) = 0; /* slot 23 */
    virtual int GetTextureCollection(int id, GMTextureInfo_t* out,
                                     unsigned int& count) = 0; /* slot 24 */
    virtual void FreeTextureCollection(int id, GMTextureInfo_t* info) = 0; /* slot 25 */

    /* Absent from both vtables -- the dispatcher bl's these directly. */
    unsigned int IsValidOptionRange(unsigned int id);
    unsigned int IsValidCollectionRange(unsigned int id);
    void SetOptionRange(unsigned int minId, unsigned int maxId);
    void SetCollectionRange(unsigned int minId, unsigned int maxId);

    /* Implicit vtable pointer occupies +0x00. */
    int m_optMin; /* +0x04 */
    int m_optMax; /* +0x08 */
    int m_colMin; /* +0x0c */
    int m_colMax; /* +0x10 */
    int m_pad14; /* +0x14 -- ctor leaves unset */
    GameVariables* m_next; /* +0x18 -- dispatcher chain */
};

typedef char GameVariablesSizeCheck[sizeof(GameVariables) == 0x1C ? 1 : -1];

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
