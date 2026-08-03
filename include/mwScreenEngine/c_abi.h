#ifndef MWSCREENENGINE_C_ABI_H
#define MWSCREENENGINE_C_ABI_H

/* C declarations for retail C++ entry points used by C translation units. */
unsigned int ScreenIntegerCompare__Fiii(int lhs, int op, int rhs);
void Free__10ScreenUtilFPv(void* pointer);
void FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
    void* self, unsigned int unused, unsigned int id, char** strings,
    unsigned int count);
void SetInt__22GameVariableDispatcherFUiUii(
    void* self, unsigned int context, unsigned int id, int value);
#endif
