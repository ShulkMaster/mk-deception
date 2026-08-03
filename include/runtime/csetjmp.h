#ifndef MKD_RUNTIME_CSETJMP_H
#define MKD_RUNTIME_CSETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

int __setjmp(void* buffer);
void longjmp(void* buffer, int value);

#ifdef __cplusplus
}
#endif

#endif
