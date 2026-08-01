#ifndef MW_MWFILE_H
#define MW_MWFILE_H

typedef int mwTargetMemAlign;

typedef struct mwFileInitParam {
    unsigned long field_00;
    short max_open_files;       /* +0x04 */
    short field_06;
    short field_08;
    short field_0A;
    unsigned long flags;        /* +0x0C */
    short field_10;
    short field_12;
    void* allocator_context;    /* +0x14 */
    unsigned long field_18;
} mwFileInitParam; /* 0x1C */

void mwFileGetDefaultInitParam(mwFileInitParam* param);
int mwFileInit(mwFileInitParam* param);
int mwFileMountPath(const char* mount, const char* path);
int mwFileSetErrorCallback(const char* mount, void* callback, void* arg);

#endif
