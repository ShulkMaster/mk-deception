#include "mwScreenEngine/ScreenParams.h"

unsigned int ScreenParams::GetCount() const {
    return m_count;
}

#pragma dont_inline on
int ScreenParams::GetType(unsigned int index) const {
    return m_entries[index]->type;
}
#pragma dont_inline off

float ScreenParams::GetFloat(unsigned int index) {
    return m_entries[index]->float_data;
}

int ScreenParams::GetInt(unsigned int index) {
    return m_entries[index]->data;
}

int ScreenParams::GetResourceID(unsigned int index) {
    return m_entries[index]->data;
}

int ScreenParams::GetBoolean(unsigned int index) {
    return m_entries[index]->data == 1;
}

ScreenNode* ScreenParams::GetScreenNode(unsigned int index) {
    /* Retail bl GetType -- keep GetType out-of-line via pragma above. */
    int type = GetType(index);
    ScreenParamRef* ref;

    /* Retail: cmpwi OBJECT(6) then NODE(5). */
    if (type == SCREEN_PARAM_OBJECT || type == SCREEN_PARAM_NODE) {
        ref = m_entries[index]->ref;
        if (ref != 0) {
            return ref->node;
        }
    }
    return 0;
}

ScreenObject* ScreenParams::GetScreenObject(unsigned int index) {
    int type = GetType(index);
    ScreenParamRef* ref;

    if (type == SCREEN_PARAM_OBJECT || type == SCREEN_PARAM_NODE) {
        ref = m_entries[index]->ref;
        if (ref != 0) {
            return ref->object;
        }
    }
    return 0;
}

const char* ScreenParams::GetScreenName(unsigned int index) {
    return m_entries[index]->string_data;
}

const char* ScreenParams::GetName(unsigned int index) {
    return m_entries[index]->string_data;
}

ScreenColor ScreenParams::GetColor(unsigned int index) {
    ScreenColor color;
    color.value = m_entries[index]->unsigned_data;
    return color;
}
