#ifndef DOLPHIN_PAD_H
#define DOLPHIN_PAD_H

#define PAD_SPEC_0 0
#define PAD_SPEC_1 1
#define PAD_SPEC_2 2
#define PAD_SPEC_3 3
#define PAD_SPEC_4 4
#define PAD_SPEC_5 5
#define PAD_MOTOR_STOP 0
#define PAD_MOTOR_RUMBLE 1
#define PAD_MOTOR_STOP_HARD 2
#define PAD_CHAN0_BIT 0x80000000UL
#define PAD_CHAN1_BIT 0x40000000UL
#define PAD_CHAN2_BIT 0x20000000UL
#define PAD_CHAN3_BIT 0x10000000UL
#define PAD_BUTTON_LEFT 0x0001
#define PAD_BUTTON_RIGHT 0x0002
#define PAD_BUTTON_DOWN 0x0004
#define PAD_BUTTON_UP 0x0008
#define PAD_TRIGGER_Z 0x0010
#define PAD_TRIGGER_R 0x0020
#define PAD_TRIGGER_L 0x0040
#define PAD_BUTTON_A 0x0100
#define PAD_BUTTON_B 0x0200
#define PAD_BUTTON_X 0x0400
#define PAD_BUTTON_Y 0x0800
#define PAD_BUTTON_MENU 0x1000
#define PAD_BUTTON_START PAD_BUTTON_MENU
#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER -1
#define PAD_ERR_NOT_READY -2
#define PAD_ERR_TRANSFER -3

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

typedef void (*PADSamplingCallback)(void);

unsigned long PADRead(PADStatus* status);
void PADClampCircle(PADStatus* status);
int PADInit(void);
int PADReset(unsigned long channels);
void PADControlMotor(signed long channel, unsigned long command);
int PADRecalibrate(unsigned long channels);
unsigned long PADRead(PADStatus* status);
void PADSetSamplingRate(unsigned long milliseconds);
void PADControlAllMotors(const unsigned long* commands);
void PADSetSpec(unsigned long spec);
unsigned long PADGetSpec(void);
int PADGetType(signed long channel, unsigned long* type);
int PADSync(void);
void PADSetAnalogMode(unsigned long mode);
PADSamplingCallback PADSetSamplingCallback(PADSamplingCallback callback);
int __PADDisableRecalibration(int disable);
int PADIsBarrel(signed long channel);

#ifdef __cplusplus
}
#endif

#endif
