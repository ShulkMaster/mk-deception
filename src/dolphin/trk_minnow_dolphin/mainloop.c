#include "dolphin/trk.h"

extern void TRKTargetInterrupt(TRKEvent* event);
extern void TRKTargetSupportRequest(void);
extern int TRKTargetStopped(void);
extern int TRKTargetContinue(void);

void TRKNubMainLoop(void)
{
    MessageBuffer* message;
    TRKEvent event;
    int shutdown_requested;
    int new_input;

    shutdown_requested = 0;
    new_input = 0;
    while (shutdown_requested == 0) {
        if (TRKGetNextEvent(&event) != 0) {
            new_input = 0;
            switch (event.event_type) {
            case 0:
                break;
            case 2:
                message = TRKGetBuffer(event.message_buffer_id);
                TRKDispatchMessage(message);
                break;
            case 1:
                shutdown_requested = 1;
                break;
            case 3:
            case 4:
                TRKTargetInterrupt(&event);
                break;
            case 5:
                TRKTargetSupportRequest();
                break;
            }
            TRKDestructEvent(&event);
            continue;
        }
        if (new_input == 0 || *gTRKInputPendingPtr != '\0') {
            new_input = 1;
            TRKGetInput();
            continue;
        }
        if (TRKTargetStopped() == 0)
            TRKTargetContinue();
        new_input = 0;
    }
}
