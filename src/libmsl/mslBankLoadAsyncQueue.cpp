#include "msl/mslBankLoadAsyncQueue.h"
#include "msl/mslBankLoadAsyncQueue_internal.h"
#include "msl/mslBank.h"
#include "runtime/cstring.h"

struct asyncRequest {
    _mslSystem* system;                   /* +0x000 */
    unsigned long flags;                  /* +0x004 */
    char filename[0x100];                 /* +0x008 */
    _mslAsyncResponse* response;          /* +0x108 */
    int state;                            /* +0x10C */
    void (*internal_callback)(_mslAsyncResponse*); /* +0x110 */
    void (*user_callback)(_mslAsyncResponse*);     /* +0x114 */
}; /* 0x118 */

struct qNode {
    asyncRequest* request;
    qNode* next;
    qNode* previous;
}; /* 0x0C */

static unsigned char requestInUse[20] = {0};
static unsigned char qNodeInUse[20] = {0};
static asyncRequest requestPool[20];
static qNode qNodePool[20];
static unsigned char okToEnter = 1;
static qNode* qHead;
static qNode* qTail;

static inline asyncRequest* requestAlloc(void) {
    asyncRequest* request = 0;
    int i;

    for (i = 0; i < 20; i++) {
        if (requestInUse[i] == 0) {
            requestInUse[i] = 1;
            request = &requestPool[i];
            memset(request, 0, sizeof(asyncRequest));
            break;
        }
    }
    return request;
}

static inline void requestFree(asyncRequest* request) {
    unsigned int index;
    asyncRequest* pool_request;

    index = ((unsigned int)request - (unsigned int)requestPool) /
            sizeof(asyncRequest);
    pool_request = &requestPool[index];
    if (request == pool_request && requestInUse[index] != 0) {
        memset(pool_request, 0xDCDCDCDC, sizeof(asyncRequest));
        requestInUse[index] = 0;
    }
}

static inline qNode* qNodeAlloc(void) {
    qNode* node = 0;
    int i;

    for (i = 0; i < 20; i++) {
        if (qNodeInUse[i] == 0) {
            qNodeInUse[i] = 1;
            node = &qNodePool[i];
            memset(node, 0, sizeof(qNode));
            break;
        }
    }
    return node;
}

static inline void qNodeFree(qNode* node) {
    unsigned int index;
    qNode* pool_node;

    index = ((unsigned int)node - (unsigned int)qNodePool) / sizeof(qNode);
    pool_node = &qNodePool[index];
    if (node == pool_node && qNodeInUse[index] != 0) {
        memset(pool_node, 0xDCDCDCDC, sizeof(qNode));
        qNodeInUse[index] = 0;
    }
}

static inline void qRemove(qNode* node) {
    if (node != 0) {
        if (node->previous != 0) {
            node->previous->next = node->next;
        }
        if (node->next != 0) {
            node->next->previous = node->previous;
        }
        if (qHead == node) {
            qHead = node->next;
        }
        if (qTail == node) {
            qTail = node->previous;
        }
        node->request = 0;
        node->next = 0;
        node->previous = 0;
        qNodeFree(node);
    }
}

static inline void closeRequest(qNode* node) {
    if (node != 0) {
        asyncRequest* request;

        request = node->request;
        if (request != 0) {
            request->response->callback = request->user_callback;
            if (request->user_callback != 0) {
                request->user_callback(request->response);
            }
            requestFree(request);
            qRemove(node);
        }
    }
}

static inline void closeKnownRequest(qNode* node, asyncRequest* request) {
    request->response->callback = request->user_callback;
    if (request->user_callback != 0) {
        request->user_callback(request->response);
    }
    requestFree(request);
    qRemove(node);
}

static inline bool qAppendToTail(void* data) {
    bool success;
    bool empty;
    qNode* node;

    node = qNodeAlloc();
    if (node != 0) {
        empty = false;
        node->request = (asyncRequest*)data;
        node->next = 0;
        node->previous = qTail;
        if (qHead == 0 && qTail == 0) {
            empty = true;
        }
        if (empty) {
            qHead = node;
            qTail = node;
        } else {
            qTail->next = node;
            qTail = node;
        }
        success = true;
    } else {
        success = false;
    }
    return success;
}

static inline bool isRespUsed(const _mslAsyncResponse* response) {
    bool used = false;
    qNode* node = qHead;
    asyncRequest* request;

    while (node != 0) {
        request = node->request;
        if (request != 0 && request->response == response) {
            used = true;
            break;
        }
        if (node != 0) {
            node = node->next;
        } else {
            node = 0;
        }
    }
    return used;
}

static inline void issueHeadRequest(void) {
    qNode* node = qHead;
    asyncRequest* request;

    if (node == 0) {
        return;
    }
    request = node->request;
    if (request == 0 || request->state != 1) {
        return;
    }

    request->state = 2;
    request->response->callback = request->internal_callback;
    mslBankLoadAsyncInternal(
        request->system, request->flags, request->filename, request->response);
}

/* Soft ceiling: mslBankLoadAsyncInternalCallback ~99.85% -- pool bases and
 * cached next node use a rotated GPR assignment; stop.
 */
void mslBankLoadAsyncInternalCallback(_mslAsyncResponse* response) {
    char filename[0x100];
    asyncRequest* request;
    int completed_state;
    void* result;
    int status;
    qNode* node;

    okToEnter = 0;
    if (qHead != 0 && response != 0 &&
        (request = qHead->request) != 0 &&
        request->response != 0 &&
        request->response == response) {
        strcpy(filename, request->filename);
        completed_state = request->state;
        result = request->response->result;
        status = request->response->status;

        if (completed_state == 3 && result != 0) {
            mslBankUnLoad((mslLoadedBank*)result);
        }

        node = qHead;
        while (node != 0) {
            qNode* next;

            if (node != 0) {
                next = node->next;
            } else {
                next = 0;
            }
            request = node->request;
            if (request != 0 && strcmp(filename, request->filename) == 0) {
                if (completed_state == 2 &&
                    (request->state == 2 || request->state == 1)) {
                    request->response->result = result;
                    request->response->status = status;
                    closeRequest(node);
                } else if (completed_state == 3 && request->state == 3) {
                    request->response->result = 0;
                    request->response->status = 4;
                    closeRequest(node);
                } else if (completed_state != 3 || request->state != 1) {
                    closeRequest(node);
                }
            }
            node = next;
        }
        issueHeadRequest();
    }
    okToEnter = 1;
}

extern "C" int mslBankLoadAsyncCancelNamed(char* filename) {
    qNode* next;
    asyncRequest* closing_request;
    int canceled = 0;
    asyncRequest* head_request;
    asyncRequest* request;
    qNode* node;

    okToEnter = 0;
    if (qHead != 0 && filename != 0 &&
        (head_request = qHead->request) != 0) {
        node = qHead;
        while (node != 0) {
            if (node != 0) {
                next = node->next;
            } else {
                next = 0;
            }
            request = node->request;
            if (request != 0 && request->response != 0 &&
                strcmp(request->filename, filename) == 0) {
                if (strcmp(request->filename, head_request->filename) == 0) {
                    request->state = 3;
                } else {
                    request->response->status = 3;
                    request->response->result = 0;
                    if (node != 0) {
                        closing_request = node->request;
                        if (closing_request != 0) {
                            closeKnownRequest(node, closing_request);
                        }
                    }
                }
                canceled++;
            }
            node = next;
        }
    }
    okToEnter = 1;
    return canceled;
}

extern "C" void mslBankLoadAsync(
    _mslSystem* system, unsigned long flags, char* filename,
    _mslAsyncResponse* response) {
    asyncRequest* request;

    okToEnter = 0;
    mslAsyncBegin(response, 0);
    request = requestAlloc();
    if (request != 0) {
        if (!isRespUsed(response)) {
            request->state = 0;
            request->system = system;
            request->flags = flags;
            request->response = response;
            strncpy(request->filename, filename, 0xFF);
            request->filename[0xFF] = 0;
            request->internal_callback = mslBankLoadAsyncInternalCallback;
            request->user_callback = request->response->callback;

            if (qAppendToTail(request)) {
                request->state = 1;
                if (qHead->request == request) {
                    issueHeadRequest();
                }
            } else {
                mslAsyncComplete(response, false, 0, (void*)1);
                requestFree(request);
            }
        }
    } else {
        mslAsyncComplete(response, false, 0, (void*)1);
    }
    okToEnter = 1;
}

typedef char async_request_size_must_be_0x118[
    sizeof(asyncRequest) == 0x118 ? 1 : -1];
typedef char qnode_size_must_be_0x0c[sizeof(qNode) == 0x0C ? 1 : -1];
