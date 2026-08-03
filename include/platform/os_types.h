#ifndef PLATFORM_OS_TYPES_H
#define PLATFORM_OS_TYPES_H
typedef int OSHeapHandle;
typedef struct OSMutex { unsigned char data[0x18]; } OSMutex;
typedef struct OSThread { unsigned char data[0x318]; } OSThread;
typedef struct OSAlarm { unsigned char data[0x28]; } OSAlarm;
#endif
