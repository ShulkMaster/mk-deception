typedef unsigned char u8;

typedef struct TRKEvent {
    int event_type;
    int event_id;
    int message_buffer_id;
} TRKEvent;

extern int TRKGetNextEvent(TRKEvent* event);
extern void* TRKGetBuffer(int buffer_id);
extern void TRKDispatchMessage(void* message);
extern void TRKTargetInterrupt(TRKEvent* event);
extern void TRKTargetSupportRequest(void);
extern void TRKDestructEvent(TRKEvent* event);
extern void TRKGetInput(void);
extern int TRKTargetStopped(void);
extern int TRKTargetContinue(void);
extern u8* gTRKInputPendingPtr;

void TRKNubMainLoop(void)
{
    void* message;
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
