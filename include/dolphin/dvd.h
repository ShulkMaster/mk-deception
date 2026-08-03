#ifndef DOLPHIN_DVD_H
#define DOLPHIN_DVD_H

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
