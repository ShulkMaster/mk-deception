#ifndef MKD_CRI_SVM_CALLBACKS_H
#define MKD_CRI_SVM_CALLBACKS_H

typedef int (*SVMServerFunction)(void* object);
typedef void (*SVMCallbackFunction)(void* object);
typedef void (*SVMErrorFunction)(void* object, char* message);

#endif
