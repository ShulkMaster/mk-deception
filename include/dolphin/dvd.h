#ifndef DOLPHIN_DVD_H
#define DOLPHIN_DVD_H

typedef struct DVDDiskID {
    char gameName[4];
    char company[2];
    unsigned char diskNumber;
    unsigned char gameVersion;
    unsigned char streaming;
    unsigned char streamingBufSize;
    unsigned char padding[22];
} DVDDiskID;

#ifdef __cplusplus
extern "C" {
#endif

int DVDCheckDisk(void);
int DVDGetDriveStatus(void);
void DVDInit(void);

#ifdef __cplusplus
}
#endif

#endif
