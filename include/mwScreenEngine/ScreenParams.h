#ifndef MWSCREENENGINE_SCREENPARAMS_H
#define MWSCREENENGINE_SCREENPARAMS_H

class ScreenNode;
class ScreenObject;

/* Param type tags used by GetScreenNode / GetScreenObject. */
enum ScreenParamType {
    SCREEN_PARAM_NODE = 5,
    SCREEN_PARAM_OBJECT = 6,
};

struct ScreenParamValue {
    int type; /* +0x00 */
    union {
        int data; /* +0x04 */
        unsigned int unsigned_data;
        float float_data;
        struct ScreenParamRef* ref;
        char* string_data;
    };
};

/*
 * When type is NODE/OBJECT: ScreenParamValue.data points here.
 * Same shape as ScreenChildEntry / SEBaseElement_t (Get* loads +0x08).
 */
struct ScreenParamRef {
    unsigned int typeTag; /* +0x00 */
    int field_0x04; /* +0x04 -- purpose not confirmed */
    union {
        void* ptr; /* +0x08 */
        ScreenNode* node;
        ScreenObject* object;
    };
};

struct ScreenColor {
    unsigned int value;
};

/*
 * ScreenParams: count at +0, then parallel pointer table at +4.
 * Each entry pointer -> ScreenParamValue { type, payload }.
 */
class ScreenParams {
public:
    unsigned int m_count; /* +0x00 */
    /* Pointer table begins immediately at +0x04 (FAM / trailing array). */
    ScreenParamValue* m_entries[1]; /* +0x04 -- indexed by Get* */

    unsigned int GetCount() const;
    int GetType(unsigned int index) const;
    float GetFloat(unsigned int index);
    int GetInt(unsigned int index);
    int GetResourceID(unsigned int index);
    int GetBoolean(unsigned int index);
    ScreenNode* GetScreenNode(unsigned int index);
    ScreenObject* GetScreenObject(unsigned int index);
    char* GetScreenName(unsigned int index);
    char* GetName(unsigned int index);
    ScreenColor GetColor(unsigned int index);
};

#endif
