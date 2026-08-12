#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwvector.h"

extern float sqrtf(float value);

typedef union RwRealBits {
    float value;
    unsigned int bits;
} RwRealBits;

typedef struct RwVectorGlobals {
    unsigned int* sqrtTable;
    unsigned int* invSqrtTable;
} RwVectorGlobals;

static RwModuleInfo vectorModule;

static void SqrtTableDestroy(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);

    if (globals->sqrtTable != 0) {
        RwEngineInstance->fpFree(globals->sqrtTable);
        globals->sqrtTable = 0;
    }
}

static int SqrtTableCreate(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    unsigned int* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    unsigned int* upperTable;
    RwRealBits input;
    RwRealBits output;
    unsigned int index;

    if (table == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return 0;
    }

    upperTable = table + 0x800;
    input.value = 1.0f;
    for (index = 0; index < 0x800; ++index) {
        output.value = sqrtf(input.value);
        output.bits -= 0x1FC00000;
        upperTable[index] = output.bits;
        input.bits += 0x1000;
    }
    for (index = 0; index < 0x800; ++index) {
        output.value = sqrtf(input.value);
        output.bits -= 0x20000000;
        table[index] = output.bits;
        input.bits += 0x1000;
    }

    globals->sqrtTable = table;
    return 1;
}

static void InvSqrtTableDestroy(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);

    if (globals->invSqrtTable != 0) {
        RwEngineInstance->fpFree(globals->invSqrtTable);
        globals->invSqrtTable = 0;
    }
}

static int InvSqrtTableCreate(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    unsigned int* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    unsigned int* upperTable;
    RwRealBits input;
    RwRealBits output;
    unsigned int index;

    if (table == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return 0;
    }

    upperTable = table + 0x800;
    input.value = 1.0f;
    for (index = 0; index < 0x800; ++index) {
        output.value = 1.0f / sqrtf(input.value);
        output.bits -= 0x20000000;
        upperTable[index] = output.bits;
        input.bits += 0x1000;
    }
    for (index = 0; index < 0x800; ++index) {
        output.value = 1.0f / sqrtf(input.value);
        output.bits -= 0x1FC00000;
        table[index] = output.bits;
        input.bits += 0x1000;
    }

    globals->invSqrtTable = table;
    return 1;
}

float _rwSqrt(float value) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        unsigned int* table = globals->sqrtTable;
        result.bits += 0x800;
        result.bits = ((result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}

float _rwInvSqrt(float value) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((unsigned char*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        unsigned int* table = globals->invSqrtTable;
        result.bits += 0x800;
        result.bits = ((~result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}








RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn,
                           const RwMatrix* matrix) {
    float x = pointIn->x;
    float y = pointIn->y;
    float z = pointIn->z;
    pointOut->x = matrix->right.x * x + matrix->up.x * y +
                  matrix->at.x * z + matrix->pos.x;
    pointOut->y = matrix->right.y * x + matrix->up.y * y +
                  matrix->at.y * z + matrix->pos.y;
    pointOut->z = matrix->right.z * x + matrix->up.z * y +
                  matrix->at.z * z + matrix->pos.z;
    return pointOut;
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn,
                            int numPoints, const RwMatrix* matrix) {
    RwV3d* result = pointsOut;
    do {
        float x = pointsIn->x;
        float y = pointsIn->y;
        float z = pointsIn->z;
        pointsOut->x = matrix->right.x * x + matrix->up.x * y +
                       matrix->at.x * z + matrix->pos.x;
        pointsOut->y = matrix->right.y * x + matrix->up.y * y +
                       matrix->at.y * z + matrix->pos.y;
        pointsOut->z = matrix->right.z * x + matrix->up.z * y +
                       matrix->at.z * z + matrix->pos.z;
        ++pointsIn;
        ++pointsOut;
    } while (--numPoints != 0);
    return result;
}

RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn,
                            const RwMatrix* matrix) {
    float x = vectorIn->x;
    float y = vectorIn->y;
    float z = vectorIn->z;
    vectorOut->x = matrix->right.x * x + matrix->up.x * y + matrix->at.x * z;
    vectorOut->y = matrix->right.y * x + matrix->up.y * y + matrix->at.y * z;
    vectorOut->z = matrix->right.z * x + matrix->up.z * y + matrix->at.z * z;
    return vectorOut;
}

void* _rwVectorClose(void* instance, int offset, int size) {
    InvSqrtTableDestroy();
    SqrtTableDestroy();
    --vectorModule.numInstances;
    return instance;
}

void* _rwVectorOpen(void* instance, int offset, int size) {
    vectorModule.globalsOffset = offset;
    if (!SqrtTableCreate()) {
        return 0;
    }
    if (!InvSqrtTableCreate()) {
        return 0;
    }
    ++vectorModule.numInstances;
    return instance;
}
