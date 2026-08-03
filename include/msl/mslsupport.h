#ifndef MSL_SUPPORT_H
#define MSL_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

void mslDebugPrintf(const char* format, ...);
void mslFileNameNoExt(const char* filename, char* output);
unsigned int mslIntLog2(unsigned int value);
float mslGetTime(void);

#ifdef __cplusplus
}
#endif

#endif
