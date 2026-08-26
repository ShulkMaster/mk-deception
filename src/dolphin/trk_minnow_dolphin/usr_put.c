typedef unsigned long u32;
typedef int BOOL;

extern void SetTRKConnected(u32 connected);
extern u32 GetTRKConnected(void);
extern void OSReport(const char* message);

BOOL usr_puts_serial(const char* message)
{
    BOOL error = 0;
    char character;
    char buffer[2];

    while (!error && (character = *message++) != '\0') {
        BOOL connected = GetTRKConnected();

        buffer[0] = character;
        buffer[1] = '\0';
        SetTRKConnected(0);
        OSReport(buffer);
        SetTRKConnected(connected);
        error = 0;
    }
    return error;
}

void usr_put_initialize(void)
{
}
