#ifndef MSL_ARAM_H
#define MSL_ARAM_H

struct mslARQRequest;

#ifdef __cplusplus
extern "C" {
#endif

void i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(
    unsigned long request_address);
void i_ARQCALLBACK_ReturnArq(unsigned long request_address);
mslARQRequest* mslGetArqRequest(void);
void mslArqRequest_Init(void);

#ifdef __cplusplus
}
#endif

#endif
