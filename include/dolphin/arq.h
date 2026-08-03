#ifndef DOLPHIN_ARQ_H
#define DOLPHIN_ARQ_H

typedef void (*ARQCallback)(unsigned long request_address);

#ifdef __cplusplus
extern "C" {
#endif

void ARQInit(void);
void ARQPostRequest(void* request, unsigned long owner, unsigned long type,
                    unsigned long priority, unsigned long source,
                    unsigned long destination, unsigned long length,
                    ARQCallback callback);

#ifdef __cplusplus
}
#endif

#endif
