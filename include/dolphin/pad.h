#ifndef DOLPHIN_PAD_H
#define DOLPHIN_PAD_H

typedef struct PADStatus {
    unsigned short button;
    signed char stickX;
    signed char stickY;
    signed char substickX;
    signed char substickY;
    unsigned char triggerLeft;
    unsigned char triggerRight;
    unsigned char analogA;
    unsigned char analogB;
    signed char err;
    unsigned char padding;
} PADStatus;

typedef char PADStatusSizeCheck[sizeof(PADStatus) == 0x0C ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

int PADRead(PADStatus* status);
void PADClampCircle(PADStatus* status);
int PADInit(void);
int PADReset(unsigned int channels);
void PADControlMotor(int channel, unsigned int command);

#ifdef __cplusplus
}
#endif

#endif
