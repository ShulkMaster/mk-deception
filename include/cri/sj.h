#ifndef CRI_SJ_H
#define CRI_SJ_H

typedef struct SJCK {
    unsigned char* data;
    int len;
} SJCK;

void SJRBF_Init(void);
void SJRBF_Finish(void);

typedef struct SJ SJ;
typedef struct SJInterface SJInterface;

struct SJInterface {
    void* reserved[3];
    void (*destroy)(SJ* sj);
    const void* (*get_uuid)(SJ* sj);
    void (*reset)(SJ* sj);
    void (*get_chunk)(SJ* sj, int channel, int max_size, SJCK* chunk);
    void (*unget_chunk)(SJ* sj, int channel, const SJCK* chunk);
    void (*put_chunk)(SJ* sj, int channel, const SJCK* chunk);
    int (*get_num_data)(SJ* sj, int channel);
    int (*is_get_chunk)(SJ* sj, int channel, int size, int* available);
    void (*entry_error_func)(SJ* sj, void* callback, void* object);
};

struct SJ {
    const SJInterface* interface;
};

typedef char SJChunkSizeCheck[sizeof(SJCK) == 0x08 ? 1 : -1];
typedef char SJInterfaceSizeCheck[sizeof(SJInterface) == 0x30 ? 1 : -1];

SJ* SJRBF_Create(void* buffer, int buffer_size, int extra_size);
int SJRBF_GetFlowCnt(SJ* sj, int channel, int counter);
SJ* SJMEM_Create(void* buffer, int buffer_size);
int SJMEM_GetBufSize(SJ* sj);

unsigned char* SJ_SearchTag(
    const SJCK* source, const char* tag, const char* terminator, SJCK* result);
void SJ_SplitChunk(const SJCK* source, int nbyte, SJCK* first, SJCK* remainder);

#endif
