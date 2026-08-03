#ifndef DOLPHIN_OS_H
#define DOLPHIN_OS_H

#include "platform/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSReport(const char* format, ...);
void OSInit(void);
void OSPanic(const char* file, int line, const char* format, ...);
int OSDisableInterrupts(void);
int OSRestoreInterrupts(int enabled);
unsigned short OSGetFontEncode(void);
int OSInitFont(void* font_data);
char* OSGetFontWidth(char* string, int* width);
char* OSGetFontTexture(char* string, void** image, int* x, int* y, int* width);
unsigned long long OSGetTime(void);
unsigned int OSGetResetCode(void);
unsigned int OSGetProgressiveMode(void);
void OSSetProgressiveMode(unsigned int mode);
unsigned char OSGetLanguage(void);
void OSCreateAlarm(OSAlarm* alarm);
void OSCancelAlarm(OSAlarm* alarm);
void OSSetPeriodicAlarm(OSAlarm* alarm, unsigned long long start,
                        unsigned long long period,
                        void (*handler)(OSAlarm*, void*));
int OSGetResetButtonState(void);
void OSResetSystem(int reset, int reset_code, int force_menu);

#ifdef __cplusplus
}
#endif

#endif
