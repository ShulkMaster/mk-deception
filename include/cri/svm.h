#ifndef MKD_CRI_SVM_H
#define MKD_CRI_SVM_H

typedef int (*SVMServerFunction)(void* object);
typedef void (*SVMCallbackFunction)(void* object);
typedef void (*SVMErrorFunction)(void* object, char* message);

void SVM_Init(void);
void SVM_Finish(void);
void SVM_Lock(void);
void SVM_Unlock(void);
void SVM_CallErr1(const char* message);
void SVM_CallErr(const char* message, ...);
unsigned int SVM_TestAndSet(int* value);
void SVM_GotoSvrBorder(int server_id);
void SVM_DelCbSvr(int server_type, int id);
int SVM_SetCbSvr(int server_type, SVMServerFunction function, void* object);
void SVM_SetCbSvrId(int server_type, int id, SVMServerFunction function,
                    void* object);
void SVM_SetCbUnlock(SVMCallbackFunction function, void* object);
void SVM_SetCbLock(SVMCallbackFunction function, void* object);
void SVM_SetCbErr(SVMErrorFunction function, void* object);
void SVM_SetCbBdr(int server_id, SVMCallbackFunction function, void* object);

#endif
