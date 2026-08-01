#ifndef MW_MWFILE_H
#define MW_MWFILE_H

typedef enum mwTargetMemAlign {
    MW_TARGET_MEM_ALIGN_DEFAULT = 0
} mwTargetMemAlign;

typedef struct mwFileInitParam {
    unsigned long field_0x00;
    short max_open_files;       /* +0x04 */
    short field_0x06;
    short field_0x08;
    short field_0x0A;
    unsigned long flags;        /* +0x0C */
    short field_0x10;
    short field_0x12;
    void* allocator_context;    /* +0x14 */
    unsigned long field_0x18;
} mwFileInitParam; /* 0x1C */

void mwFileGetDefaultInitParam(mwFileInitParam* param);
int mwFileInit(mwFileInitParam* param);
int mwFileMountPath(const char* mount, const char* path);
int mwFileSetErrorCallback(const char* mount, void* callback, void* arg);

#endif
